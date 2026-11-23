# Quick Start

<!-- md-trans-meta sourceCommit=ba12e592e8f84bc59709464bd3ad6ca010498d8b translatedAt=2026-11-06T11:40:38.080Z pushedAt=2026-11-16T02:09:26.902Z nl=2 -->

## Converting an ONNX Model Using ATC

This section describes how to use the ATC command to convert a model file in the .onnx format and perform inference on an AI processor.

For model files exported by frontend frameworks such as PyTorch, TensorFlow, and MindSpore, GE provides the model conversion capability. It supports using the ATC command-line tool to convert a model into an offline model adapted to the AI processor and perform inference in graph mode. ATC currently supports the following:

- Converting a model in the \*.onnx format into a model in the \*.om format
- Converting a model in the \*.pb format into a model in the \*.om format
- Converting a model in the \*.air format into a model in the \*.om format
- Converting a model in the \*.prototxt or \*.caffemodel format into a model in the \*.om format

The following uses the conversion of a model in the \*.onnx format into a model in the \*.om format as an example to describe the basic usage:

1. Obtain the ONNX network model.

    Click [Link](https://github.com/onnx/models/blob/main/Computer_Vision/resnet50_Opset16_timm/resnet50_Opset16.onnx) to download the file, and then, as a user running the CANN software package, upload the obtained file to any directory in the development environment, for example, the $HOME/module/ directory.

2. Use the ATC CLI tool for model conversion (using the \*.onnx model as an example).

    ```bash

    atc --model=$HOME/module/resnet50.onnx --framework=5 --output=$HOME/module/out/onnx_resnet50 --soc_version=<soc_version>
    ```

    - --model: path and name of the original network model file.
    - --framework: framework type of the original network model. "3" indicates a TensorFlow model, "5" indicates an ONNX model, and "1" indicates a model file in the MindSpore framework \*.air format or a standard \*.air format file exported by TorchAir.
    - --output: path and name for storing the converted offline model. For example, if it is set to `$HOME/module/out/tf_resnet50`, the converted offline model is stored in `$HOME/module/out/` and named "tf\_resnet50.om".
    <!-- npu="950,A3,910b,910,310p,310b" id1 -->
    - --soc\_version: model of the AI processor. To query the value, perform the following steps:
        <!-- npu="910b,910,310p,310b" id3 -->
        - For the following products: run the `npu-smi info` command on the server where the AI processor is installed to obtain the **Name** information. The actual configuration value is AscendName. For example, if the **Name** value is `xxxyy`, the actual configuration value is `Ascendxxxyy`.

            Atlas A2 series products

            Atlas 200I/500 A2 inference products

            Atlas inference series products

            Atlas training series products
        <!-- end id3 -->
        <!-- npu="A3" id2 -->
        - For Atlas A3 series products, run the `npu-smi info -t board -i id -c chip_id` command on the server where the AI processor is installed to query and obtain the **Chip Name** and **NPU Name** information. The actual configuration value is Chip Name\_NPU Name. For example, if the **Chip Name** value is `Ascendxxx` and the **NPU Name** value is 1234, the actual configuration value is `Ascendxxx_1234`. Where:
            - id: device ID. The NPU ID obtained by running the **npu-smi info -l** command is the device ID.
            - chip\_id: chip ID. The Chip ID obtained by running the **npu-smi info -m** command is the chip ID.
        <!-- end id2 -->
        <!-- npu="950" id4 -->
        - For Ascend 950PR&950DT series products, run the `npu-smi info -t board -i id` command on the server where the AI processor is installed to query and obtain the **Chip Name** and **NPU Name** information. The actual configuration value is Chip Name\_NPU Name. For example, if the **Chip Name** value is `Ascendxxx` and the **NPU Name** value is 1234, the actual configuration value is `Ascendxxx_1234`.

            Where id is the device ID. The NPU ID obtained by running the **npu-smi info -l** command is the device ID.
        <!-- end id4 -->
    <!-- end id1 -->

    <!-- @ref: ge/res/docs/zh/user_guides/graph_dev/quick_start_res.md#id1 -->

    For more parameters and detailed usage, see [Ascend Tensor Compiler (ATC)](../../atc_tools/README.md).

3. If the following information is displayed, the model conversion is successful.

   ```bash
   ATC run success, welcome to the next use.
   ```

   After the command is successfully executed, the \*.onnx model is converted into an \*.om offline model adapted to the AI processor. In the path specified by the --output parameter, you can view the generated \*.om offline model file.

4. Follow-up operations.

   After the model is converted into an \*.om offline model, you can call the model loading API to load the converted \*.om offline model, and then call the model execution API for inference. The following is a brief process of loading a model file from a file and performing inference. For details about model loading and inference, see the "Model Inference" section in [Application Development](https://gitcode.com/cann/docs/blob/master/docs/en/app-dev/c/00_acl_cpp_dev.md).

  ```c++
  // 1. Specify the model path.
  const char* omModelPath = "./model/onnx_resnet50.om";
  uint32_t modelId;
  // 2. Load the offline model file. If the model is loaded successfully, the ID of the model is returned.
  ret = aclmdlLoadFromFile(omModelPath, &modelId);
  // 3. Execute the model. inputDs and outputDs are the Dataset objects for model input and output.
  ret = aclmdlExecute(modelId, inputDs, outputDs);
  // 4. Unload the model.
  ret = aclmdlUnload(modelId);
  ```

## Building a Graph from Scratch Using APIs

The C++ graph engine APIs of GE provide two ways to construct a graph:

- **Using graph engine APIs to construct a new graph**: You add operators to the compute graph one by one through the graph engine APIs to construct a compute graph represented by Ascend IR.
- **Using the Parser API to parse an original model into a graph**: You use the Parser API to parse a framework model file (such as \*.onnx and \*.pb formats) into a compute graph represented by Ascend IR.

This section combines the preceding two scenarios to describe how to construct a graph. After the graph is constructed, you can modify it as required and then compile it into an offline model adapted to the AI processor. The overall process is as follows:

![](../figures/overall_process.png)

The process is described as follows:

1. Before constructing a graph, set up the environment, install the corresponding CANN software package, and analyze the network structure. If there are unsupported operators, develop the corresponding operators and deploy them to the hardware environment.
    1. If you **use graph engine APIs to construct a new graph**, determine the following information based on the original network:
        1. The operators contained in the network, as well as the inputs, outputs, attributes, and other information of these operators.
        2. The relationships between operators in the network.
        3. Check whether the operators in the original network are supported by the AI processor. For the currently supported operators, see the "Ascend IR Operator Specifications" section in [Operator Library](https://hiascend.com/document/redirect/CannCommunityOplist). If an operator is not supported or does not meet the actual requirements, developers can customize an Ascend C operator by referring to [Ascend C Operator Development](https://gitcode.com/cann/asc-devkit/blob/master/docs/en/guide/index.md), or customize a TBE operator by referring to [TBE & AI CPU Operator Development](https://hiascend.com/document/redirect/CannCommunityOpdevWizard). After selecting one of the methods, deploy the operator to the hardware environment.

    2. If **the Parser API is used to parse the original model into a Graph**, check whether the operators in the original network are supported by the AI processor. For the currently supported operators, see the "Ascend IR Operator Specifications" section in [Operator Library](https://hiascend.com/document/redirect/CannCommunityOplist). If an operator is not supported or does not meet the actual requirements, customize an Ascend C operator by referring to [Ascend C Operator Development](https://gitcode.com/cann/asc-devkit/blob/master/docs/en/guide/index.md), or customize a TBE operator by referring to [TBE & AI CPU Operator Development](https://hiascend.com/document/redirect/CannCommunityOpdevWizard). After selecting one of the methods, deploy the operator to the hardware environment.

2. Build a Graph. Developers can build a Graph from scratch using the graph engine API, or parse the original model into a Graph using the Parser API.
3. Modify the Graph. If developers want to optimize the Graph structure, they can directly modify the built Graph into the desired structure.
4. Compile and run the Graph. Developers can compile the modified Graph into an offline model adapted to the AI processor, and then load the model through the model loading API for on-board inference. Alternatively, they can directly run the Graph to obtain the running result.

### Obtaining the Sample

Click [Build a Graph Using APIs](https://gitee.com/ascend/samples/tree/master/cplusplus/level1_single_api/3_ir/IRBuild) to obtain the sample code. The directory structure is as follows:

```text
├── src
│   ├──main.cpp                // Implementation file
├── Makefile                    // Compilation script
├── CMakeLists.txt              // Compilation script
├── data
│   ├──data_generate.py        // Used to generate the data required by the Graph, such as weights and biases, when building a Graph from scratch using the graph engine API
│   ├──tensorflow_generate.py  // Used to generate a TensorFlow model in .pb format when parsing an original TensorFlow model into a Graph using the Parser API
│   ├──caffe_generate.py       // Used to generate a Caffe model in .pbtxt format and a weight file in .caffemodel format when parsing an original Caffe model into a Graph using the Parser API
├── scripts
│   ├──host_version.conf       // Version configuration
│   ├──testcase_300.sh         // Test script
```

The sample provides the following functions:

1. Build a graph.
    - **Use graph engine APIs to construct a new graph**

        The graph structure constructed in this sample is as follows:

        ![Graph structure](../figures/ir_build_image1.png)

    - **Use the Parser API to parse the original model into a graph**. TensorFlow is used as an example here.

        The TensorFlow original model structure provided in this sample is as follows:

        ![TensorFlow original model structure](../figures/ir_build_image2.png)

        The parsed graph structure is as follows:

        ![Parsed graph structure](../figures/ir_build_image3.png)

2. Modify the graph: This example uses the graph obtained by parsing a TensorFlow model through the Parser API.

    The modified graph structure is as follows, with an Abs operator inserted between the Add operator and the Const operator:

    ![Inserting the Abs operator](../figures/ir_build_image4.png)

3. Compile the graph: Compile the final graph above into an offline model.

### Compiling and Running

1. Prepare the environment.
    - See [Environment Preparation](environment_setup.md) to set up the environment and configure environment variables.
    - To parse the original model in the TensorFlow framework into a Graph, set up the TensorFlow 1.15.0 environment. The following is an example of installing it as the root user on the x86 architecture:

        ```python
        pip3 install tensorflow-cpu==1.15
        ```

        If you install it as a non-root user, add the --user parameter to the end of the preceding command. If the architecture is aarch64, see [TensorFlow 1.15 Model Migration](https://hiascend.com/document/redirect/tfcommunityguide) \> to install the open-source framework TensorFlow 1.15.

2. Prepare the graph construction data.
    - **Build a Graph from scratch using the graph engine API**.

        Run the data generation script in the data directory. The command is as follows:

        ```python
        python3 data_generate.py
        ```

        After the execution is complete, data in .bin format is generated in the data directory. During subsequent model construction, data such as weights and biases is read from this file.

    - **Parse the original model into a Graph using Parser APIs**.

        Run the original model generation script in the data directory. The command is as follows:

        ```python
        python3 tensorflow_generate.py
        ```

        After the execution is complete, a model file in .pb format is generated in the data directory. This model will be used later when parsing the original model, and its name is tf\_test.pb.

3. Compile the code.
    1. (Optional) Open the Makefile and change the following **ASCEND\_PATH** path to the CANN software installation directory /cann. For example, for the root user, change it to /usr/local/Ascend/cann.

        ```makefile
        ifeq (${ASCEND_INSTALL_PATH},)
             ASCEND_PATH := /usr/local/Ascend/cann
        ```

    2. Perform compilation.

        Run the following command in the same directory as the Makefile to compile:

        ```bash
        make clean && make ir_build
        ```

        After the compilation is complete, the executable file **ir\_build** is generated in the out folder in the current path.

4. Run the code.

    - **Build a Graph from scratch using the graph engine API**.

        Run the following command in the out folder:

        ```bash
        ./ir_build <soc_version> gen
        ```

        If the following information is displayed, the execution is successful, and an \*.om offline model file is generated in the same directory.

        ```text
        ========== Generate Graph1 Success!==========
        Build Model1 SUCCESS!
        Save Offline Model1 SUCCESS!
        ```

    - **Use Parser APIs to parse the original model into a Graph**.

        Run the following command in the out folder:

        ```bash
        ./ir_build <soc_version> tf
        ```

        For details about how to query the value of <soc\_version\>, see [Converting an ONNX Model Using ATC](#converting-an-onnx-model-using-atc).
        If the following information is displayed, the execution is successful, and an \*.om offline model file is generated in the same directory.

        ```text
        ========== Generate graph from tensorflow origin model success.==========
        Modify Graph Start.
        Find src node: const.
        Find dst node: add.
        Modify Graph Success.
        ========== Modify tensorflow origin graph success.==========
        Build Model1 SUCCESS!
        Save Offline Model1 SUCCESS!
        ```

5. Follow-up operations.

    After the model is converted into an \*.om offline model, you can call the model loading API to load the converted \*.om offline model, and then call the model execution API for inference. For details about model loading and inference, see "Model Inference" in [Application Development](https://gitcode.com/cann/docs/blob/master/docs/en/app-dev/c/00_acl_cpp_dev.md).

### Code Analysis

After compiling and running the quick start sample, if you want to understand the implementation logic of the code, see the code analysis below. This section describes the code logic by function. To facilitate reading and understanding, the variables and functions related to each function are described together.

Open the "src/main.cpp" file, start with the main function to understand the code connection logic of the entire sample, and then understand the implementation of each custom function.

1. The main function connects the entire code logic for building a Graph.

    ```c++
    int main(int argc, char* argv[])
    {
        // 1. Build a Graph
        Graph graph1("IrGraph1");
           // 1.1 Build a Graph from scratch using the graph engine API
            ret = GenGraph(graph1);

           // 1.2 Parse the original model into a Graph using the Parser API
            std::string tfPath = "../data/tf_test.pb";
            std::map<AscendString, AscendString> parser_options;
            tfStatus = ge::aclgrphParseTensorFlow(tfPath.c_str(), parser_options, graph1);

           // 2. Modify the Graph: here, the Graph parsed from a TensorFlow model is used as an example
            ModifyGraph(graph1);

        // 3. Initialize the system and apply for resources
        std::map<AscendString, AscendString> global_options = {
            {AscendString(ge::ir_option::SOC_VERSION), AscendString(argv[kSocVersion])}  ,
        };
        auto status = aclgrphBuildInitialize(global_options);

        // 4. Compile the Graph
        ModelBufferData model1;
        std::map<AscendString, AscendString> options;
        PrepareOptions(options);
        status = aclgrphBuildModel(graph1, options, model1);

        // 5. Save the model in the memory buffer as an offline model file.
        status = aclgrphSaveModel("ir_build_sample1", model1);

        // 6. Release resources.
        aclgrphBuildFinalize();

    }
    ```

2. Include the dependent header files.

    ```c++
    #include <string.h>
    #include "tensorflow_parser.h"
    #include "graph.h"
    #include "types.h"
    #include "tensor.h"
    #include "attr_value.h"
    #include "ge_error_codes.h"
    #include "ge_api_types.h"
    #include "ge_ir_build.h"
    #include "all_ops.h"
    ... ...
    using namespace std;
    using namespace ge;
    using ge::Operator;
    ```

3. Build the graph.

    Define operators using the operator prototype derivation API and connect the operators. The detailed code example is as follows:

    ```c++
    bool GenGraph(Graph& graph)
    {
        auto shape_data = vector<int64_t>({ 1,1,28,28 });
        TensorDesc desc_data(ge::Shape(shape_data), FORMAT_ND, DT_FLOAT16);
        // Create a Data operator instance.
        auto data = op::Data("data");
        data.update_input_desc_x(desc_data);
        data.update_output_desc_y(desc_data);

        // Create an Add operator instance and connect it to the Data operator.
        auto add = op::Add("add")
            .set_input_x1(data)
            .set_input_x2(data);

        // Create an AscendQuant operator instance and connect it to the Data operator.
        auto quant = op::AscendQuant("quant")
            .set_input_x(data)
            .set_attr_scale(1.0)
            .set_attr_offset(0.0);

        auto conv_weight = op::Const("Conv2D/weight")
            .set_attr_value(weight_tensor);
        // Create a Conv2D operator instance and connect it to the AscendQuant operator.
        auto conv2d = op::Conv2D("Conv2d1")
            .set_input_x(quant)
            .set_input_filter(conv_weight)
            .set_attr_strides({ 1, 1, 1, 1 })
            .set_attr_pads({ 0, 1, 0, 1 })
            .set_attr_dilations({ 1, 1, 1, 1 });
            ... ...

        auto dequant_scale = op::Const("dequant_scale")
            .set_attr_value(dequant_tensor);
        // Create an AscendDequant operator instance and connect it to the Conv2D operator.
        auto dequant = op::AscendDequant("dequant")
            .set_input_x(conv2d)
            .set_input_deq_scale(dequant_scale);
            ... ...

        auto bias_weight_1 = op::Const("Bias/weight_1")
            .set_attr_value(weight_bias_add_tensor_1);
        // Create a BiasAdd operator instance and connect it to the AscendDequant operator
        auto bias_add_1 = op::BiasAdd("bias_add_1")
            .set_input_x(dequant)
            .set_input_bias(bias_weight_1)
            .set_attr_data_format("NCHW");
            ... ...

        auto dynamic_const = op::Const("dynamic_const").set_attr_value(dynamic_const_tensor);
        // Create a Reshape operator instance and connect it to the BiasAdd operator
        auto reshape = op::Reshape("Reshape")
            .set_input_x(bias_add_1)
            .set_input_shape(dynamic_const);
            ... ...

        auto matmul_weight_1 = op::Const("dense/kernel")
            .set_attr_value(matmul_weight_tensor_1);
        // Create a MatMul operator instance and connect it to the Reshape operator
        auto matmul_1 = op::MatMul("MatMul_1")
            .set_input_x1(reshape)
            .set_input_x2(matmul_weight_1);
            ... ...

        auto bias_add_const_1 = op::Const("dense/bias")
            .set_attr_value(bias_add_const_tensor_1);
        // Create a BiasAdd operator instance and connect it to the MatMul operator
        auto bias_add_2 = op::BiasAdd("bias_add_2")
            .set_input_x(matmul_1)
            .set_input_bias(bias_add_const_1)
            .set_attr_data_format("NCHW");
        // Create a Relu6 operator instance and connect it to the BiasAdd operator
        auto relu6 = op::Relu6("relu6")
            .set_input_x(bias_add_2);
            ... ...

        auto matmul_weight_2 = op::Const("OutputLayer/kernel")
            .set_attr_value(matmul_weight_tensor_2);
        // Create a MatMul operator instance and connect it to the Relu6 operator
        auto matmul_2 = op::MatMul("MatMul_2")
            .set_input_x1(relu6)
            .set_input_x2(matmul_weight_2);
            ... ...

        auto bias_add_const_3 = op::Const("OutputLayer/bias")
            .set_attr_value(bias_add_const_tensor_3);
        // Create a BiasAdd operator instance and connect it to the MatMul operator
        auto bias_add_3 = op::BiasAdd("bias_add_3")
            .set_input_x_by_name(matmul_2, "y")
            .set_input_bias_by_name(bias_add_const_3, "y")
            .set_attr_data_format("NCHW");
        // Create a SoftmaxV2 operator instance and connect it to the BiasAdd operator
        auto softmax = op::SoftmaxV2("Softmax")
            .set_input_x_by_name(bias_add_3, "y");
        std::vector<Operator> inputs{ data };

        std::vector<Operator> outputs{ softmax, add };
        std::vector<std::pair<ge::Operator, std::string>> outputs_with_name = {{softmax, "y"}};
        // Call the API to set the inputs and outputs of the graph
        graph.SetInputs(inputs).SetOutputs(outputs);
        return true;
    }
    ```

4. Modify the graph: The following uses the graph parsed from a TensorFlow model as an example for modification.

    Insert an Abs operator between the Add operator and the Const operator:

    ```c++
    bool ModifyGraph(Graph &graph) {

        const std::string CONST = "const1";
        const std::string ADD = "input3_add";
        GNode src_node;
        GNode dst_node;
        // Obtain all nodes in the graph
        std::vector<GNode> nodes = graph.GetAllNodes();
        graphStatus ret = GRAPH_FAILED;
        for (auto &node : nodes) {
            ge::AscendString name;
            // Obtain the name of the operator in the graph
            ret = node.GetName(name);

            std::string node_name(name.GetString());
        }
        // Delete the specified connection edge in the graph
        ret = graph.RemoveEdge(src_node, 0, dst_node, 1);

        auto abs = op::Abs("input3_abs");

        // Add an abs node to the graph
        GNode node_abs = graph.AddNodeByOp(abs);

       //Add a data edge to connect the newly added node
        ret = graph.AddDataEdge(src_node, 0, node_abs, 0);

        ret = graph.AddDataEdge(node_abs, 0, dst_node, 1);
        return true;
    }
    ```

5. Compile the graph.

    After the graph is built and modified, call the API to compile the graph. The API call process is shown in the following figure. For details, see the main\(\) function.

    ![Graph construction process](../figures/ir_graph_construction.png)
