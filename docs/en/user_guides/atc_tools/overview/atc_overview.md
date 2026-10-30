# ATC Overview

<!-- md-trans-meta sourceCommit=bd3de52c8a808742e9c01b524792105f46cad44c translatedAt=2026-10-27T14:25:37.096Z pushedAt=2026-10-28T09:04:53.488Z nl=2 -->

## ATC Tool Introduction

This section describes the functional architecture of the ATC tool and the terms and abbreviations you may encounter when using it.

### ATC Overview

Ascend Tensor Compiler (ATC) is a model conversion tool in the heterogeneous computing architecture CANN. It converts network models from open-source frameworks and single-operator description files (in JSON format) defined by Ascend IR into .om offline models supported by AI processors. Its functional architecture is shown in [Figure 1](#fig1).

During model conversion, ATC performs specific operations such as operator scheduling optimization, weight data rearrangement, and memory usage optimization to further tune the original deep learning model, thereby meeting the high-performance requirements in deployment scenarios and enabling efficient execution on AI processors.

**Figure 1** ATC functional architecture<a id="fig1"></a>
![Diagram](../figures/atc_tool_func_architecture.png "ATC functional architecture")

Where:

- Open-source framework network model scenario:
    1. The network model from an open-source framework is parsed by Parser and converted into an intermediate IR Graph.
    2. After a series of operations such as graph preparation, graph splitting, graph optimization, and graph compilation, the intermediate IR is converted into an offline model adapted to AI processors (here, "graph" refers to the network model topology).
    3. The converted offline model is uploaded to the board environment. The model file is loaded through the **model loading API**, and then the **model execution API** is called to implement inference. For the detailed flow, see the "Model Inference" section in *[Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md)*.

- Single-operator description file scenario:

    The single-operator description file (in JSON format) defined by Ascend IR is compiled by ATC into a single-operator offline model adapted to AI processors. The model is then uploaded to the board environment, and the **single-operator model loading API** is used to load the single-operator model file for verifying single-operator functions. For the detailed flow, see [Single-Operator Model Execution](../../../api/graph_engine_api/c/acl/single_operator_model_execute.md).

    For details about the configuration of the single-operator description file, see [Single-Operator Model Conversion](../sinlgeop_model_convert/sinlgeop_model_convert.md).

### Key Concepts

- **GE**

    Graph Engine, the control center for computational graph compilation and execution, provides functions such as graph optimization, graph compilation management, and graph execution control. GE supports multiple AI frameworks through a unified graph development interface, enabling computational graphs from different AI frameworks to be converted into Ascend graphs. During original graph optimization, GE performs whole-graph optimization internally.

- **YUV420SP**

    A lossy image color encoding format, commonly available in two formats: YUV420SP\_UV and YUV420SP\_VU.

- **Data Layout Format (Format)**

    Format is the physical layout format of data, which defines the dimensions for interpreting data, such as 1D, 2D, 3D, 4D, and 5D.

- **NCHW and NHWC**

    In deep learning frameworks, multidimensional data is stored through multidimensional arrays. For example, the feature map of a convolutional neural network is usually stored in a four-dimensional array, that is, 4D. The 4D format is explained as follows:

  - N: Batch, the number of images, for example.
  - H: Height, the height of the feature map, that is, the number of pixels in the vertical direction.
  - W: Width, the width of the feature map, that is, the number of pixels in the horizontal direction.
  - C: Channels, the channels of the feature map. For example, the number of channels of a color RGB image is 3.

    Since data can only be stored linearly, these four dimensions have a corresponding order. Different deep learning frameworks store feature map data in different orders. For example, in Caffe, the order is \[Batch, Channels, Height, Width\], that is, NCHW; in TensorFlow, the order is \[Batch, Height, Width, Channels\], that is, NHWC.

    As shown in [Figure 2](#fig2), taking an RGB image as an example, in NCHW, C is arranged in the outer layer. Within each channel, pixels are adjacent to each other, and the actual storage is "RRRRRRGGGGGGBBBBBB", that is, all pixel values of the same channel are stored sequentially together. In NHWC, C is arranged in the innermost layer. Within each channel, pixels are spaced apart, and the actual storage is "RGBRGBRGBRGBRGBRGB", that is, the pixel values at the same position across multiple channels are stored sequentially together.

    **Figure 2**  NCHW and NHWC<a id="fig2"></a>
    ![](../figures/nchw_and_nhwc.png "NCHW and NHWC")

- **NC1HWC0**

    In AI processors, to improve the access efficiency of general matrix multiplication (GEMM) operation data blocks, all tensor data uniformly adopts the five-dimensional NC1HWC0 data format.

    C0 is strongly related to the microarchitecture. It is the amount of data on one side processed by a matrix unit. A matrix unit processes 32B\*32B data, and one side is 32B. For example, when the data type is float16 (2 bytes), C0 = 32/2 = 16; when the data type is float32 (4 bytes), C0 = 32/4 = 8.

    C1=\(C+C0-1\)/C0, if the result is not divisible, round down.

    The conversion from NHWC/NCHW -\> NC1HWC0 is as follows: split the data along the C dimension into C1 parts of NHWC0/NC0HW, and then arrange the C1 parts of NHWC0/NC0HW contiguously in memory to form NC1HWC0. The following figure shows the format conversion.

    **Figure 3**  NC1HWC0<a id="fig3"></a>
    ![](../figures/nc1hwc0.png "NC1HWC0")

  - The conversion formula from NHWC -\> NC1HWC0 is as follows:

        ```
        Tensor.reshape( [N, H, W, C1, C0]).transpose( [0, 3, 1, 2, 4] )
        ```

  - The conversion formula from NCHW -\> NC1HWC0 is as follows:

        ```
        Tensor.reshape( [N, C1, C0, H, W]).transpose( [0, 1, 3, 4, 2] )
        ```

- **FRACTAL\_Z**

    FRACTAL\_Z is the data format used to define convolution weights, obtained by transforming the FT Matrix (FT: Filter, convolution kernel). FRACTAL\_Z is the final data format sent to the Cube, using a 4-dimensional data layout of "C1HW,N1,N0,C0".

    The data has two layers of Tiling, as shown in the following figure:

    ![](../figures/fractal_z_image.png)

    The first layer is related to the Cube size, and the data is contiguous along the column direction (small n). The second layer is related to the matrix size, and the data is contiguous along the row direction (large Z).

    For example: HWCN = \(2, 2, 32, 32\), convert it to FRACTAL\_Z\( C1HW, N1, N0, C0 \) = \(8, 2, 16, 16\).

    The process of transforming HWCN to FRACTAL\_Z is as follows:

    ```python
    Tensor.padding([ [0,0], [0,0], [0,(C0–C%C0)%C0], [0,(N0–N%N0)%N0] ]).reshape( [H, W, C1, C0, N1, N0]).transpose( [2, 0, 1, 4, 5, 3] ).reshape( [C1*H*W, N1, N0, C0])
    ```

    The process of transforming NCHW to FRACTAL\_Z is as follows:

    ```python
    Tensor.padding([ [0,(N0–N%N0)%N0], [0,(C0–C%C0)%C0], [0,0], [0,0] ]).reshape( [N1, N0, C1, C0, H, W,]).transpose( [2, 4, 5, 0, 1, 3] ).reshape( [C1*H*W, N1, N0, C0])
    ```

- **FRACTAL\_NZ**

    FRACTAL\_NZ is a fractal format. For example, in the data storage of a Feature Map, when computing in a cube unit, the data format of the output matrix is NW1H1H0W0. The entire matrix is divided into (H1\*W1) fractals, arranged in column-major order, with a shape resembling the letter N. Each fractal contains (H0\*W0) elements, arranged in row-major order, with a shape resembling the letter z. Considering the data layout, the NW1H1H0W0 data format is called the Nz (large N, small z) format. Here, H0 and W0 represent the size of a fractal. The following figure shows the layout:

    ![](../figures/nz_convert.png)

    The transformation process of ND –\> FRACTAL\_NZ is as follows:

    ```python
    (..., N, H, W )->pad->(..., N, H1*H0, W1*W0)->reshape->(..., N, H1, H0, W1, W0)->transpose->(..., N, W1, H1, H0, W0)
    ```

## Call Process

Before running the ATC tool, you need to prepare the environment and model. This section describes the execution flow of the ATC tool and its interaction flow with each component.

### Execution Flow

The execution flow is shown in [Figure 1](#fig3).

**Figure 1** Execution flow<a id="fig3"></a>
![](../figures/runtime_flow.png "execution flow")

1. Before using the ATC tool, install the CANN software package in the development environment, obtain the ATC tool from the relevant path, and then set the environment variables. For details, see [Preparing the Environment](environment_setup.md).
2. Prepare the model or single-operator description file to be converted, and upload it to the development environment. For details about the configuration of the single-operator description file, see [Single-Operator Model Conversion](../sinlgeop_model_convert/sinlgeop_model_convert.md).
3. Use the ATC tool for model conversion. For details about the parameters used during model conversion, see [Parameter Description](../CLI_options/README.md).

### Model Conversion Interaction Flow

The following uses the conversion of an open-source framework network model into an om offline model as an example to describe in detail the interaction flow with surrounding modules during model conversion.

Operators in a network model are classified into AI Core operators and AI CPU operators based on their computing units: AI Core operators are executed on the core computing units of the AI processor, while AI CPU operators are executed on the AI CPU computing units.

In the model conversion interaction flows of AI Core operators and AI CPU operators, although both involve stages such as graph preparation, graph splitting, graph optimization, and graph compilation, the internal modules involved in the interaction differ because their computing units are different. See the following figures.

For details about operator types and basic concepts, see *[Ascend C Operator Development](https://gitcode.com/cann/asc-devkit/blob/9.2.0/docs/zh/guide/index.md)* or *[TBE & AI CPU Operator Development](https://hiascend.com/document/redirect/CannCommunityOpdevWizard)* (choose either supported method). If the network model you use contains custom operators, develop and deploy the custom operators by referring to the preceding manuals first. During model conversion, the custom operator library is searched first to match the operators in the model file. If the matching fails, the built-in operator library is searched.

During model conversion, if compilation fails because an AI CPU operator does not support a certain data type, you can enable the Cast operator auto-insertion feature to quickly convert the input to a data type supported by the operator, thereby quickly bringing up the network. For the detailed flow, see [Enabling the AI CPU Cast Operator Auto-Insertion Feature](../references/enable_ai_cpu_cast_auto_insert.md).

- AI Core Operator Model Conversion Interaction Flow

    **Figure 2**  AI Core operator model conversion interaction flow
    ![](../figures/ai_core_op_model_conv_flow.png "AI Core operator model conversion interaction flow")

    1. Call the framework Parser to convert the model format of mainstream frameworks into the CANN model format.
    2. Graph preparation stage: In this stage, functions such as original graph optimization and Infershape inference (setting the shape and dtype of operator outputs) are completed.
    3. Graph splitting phase: GE (Graph Engine) splits the graph into multiple subgraphs based on the engine.
    4. Graph optimization phase: GE optimizes the split subgraphs. During optimization, it precompiles AI Core operators and performs UB (Unified Buffer) fusion according to the current subgraph flow, then finds the operator implementation based on the operator information in the operator information library and compiles it into an operator kernel (the \*.o and \*.json of the operator). Finally, it returns the optimized subgraphs to GE. The optimized subgraphs are merged into a complete graph, which then undergoes whole-graph optimization.

        <!-- npu="950" id1 -->
        The Ascend 950PR&950DT products do not support UB fusion.
        <!-- end id1 -->

    5. Graph compilation phase: GE performs graph compilation, including memory allocation and stream resource allocation. After graph compilation is complete, an offline model file adapted to the AI processor is generated (\*.om).

- AI CPU operator model conversion interaction flow

    **Figure 3**  AI CPU operator model conversion interaction flow
    ![](../figures/ai_cpu_op_model_conv_flow.png "AI-CPU operator model conversion interaction flow")

    1. Invoke the framework Parser function to convert the model format of mainstream frameworks into the CANN model format.
    2. Graph preparation phase: This phase completes basic parameter validation of operators and Infershape inference (setting the shape and dtype of operator outputs).

        In addition, GE sends the complete graph to the AI CPU Engine. The AI CPU Engine reads the operator information library, matches the formats supported by the operator, and returns the format to GE.

    3. Graph splitting phase: GE splits the graph into multiple subgraphs based on the engine.
    4. Graph optimization phase: GE sends the split subgraphs to the AI CPU Engine. The AI CPU Engine optimizes the subgraphs and returns the optimized subgraphs to GE. The optimized subgraphs are merged into a complete graph, which then undergoes whole-graph optimization.

    5. Graph compilation: GE performs graph compilation, including memory allocation and stream resource allocation, and sends a genTask request to AI CPU Engine. AI CPU Engine returns the taskinfo of the operator to GE. After graph compilation is complete, an offline model file (\*.om) adapted to the AI processor is generated.
