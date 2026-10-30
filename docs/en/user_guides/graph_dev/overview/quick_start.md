# Quick Start

<!-- md-trans-meta sourceCommit=bd3de52c8a808742e9c01b524792105f46cad44c translatedAt=2026-10-21T10:08:35.384Z pushedAt=2026-10-28T11:08:46.546Z nl=2 -->

## Converting an ONNX Model Using ATC

This section describes how to use the ATC command to convert a model file in the .onnx format and perform inference on the AI processor.

For model files exported by frontend frameworks such as PyTorch, TensorFlow, and MindSpore, GE provides the model conversion capability. It supports using the ATC command-line tool to convert a model into an offline model adapted to the AI processor and perform inference in graph mode. ATC currently supports the following conversions:

- Converting a model in the \*.onnx format to a model in the \*.om format
- Converting a model in the \*.pb format to a model in the \*.om format
- Converting a model in the \*.air format to a model in the \*.om format
- Converting a model in the \*.prototxt or \*.caffemodel format to a model in the \*.om format

The following uses the conversion of a model in the \*.onnx format to a model in the \*.om format as an example to describe the basic usage:

1. Obtain the ONNX network model.

    Click [Link](https://github.com/onnx/models/blob/main/Computer_Vision/resnet50_Opset16_timm/resnet50_Opset16.onnx) to download the file, and then, as a user running the CANN software package, upload the obtained file to any directory in the development environment, for example, the $HOME/module/ directory.

2. Use the ATC CLI tool to convert the model (using the \*.onnx model as an example).

    ```bash

    atc --model=$HOME/module/resnet50.onnx --framework=5 --output=$HOME/module/out/onnx_resnet50 --soc_version=<soc_version>
    ```

    - --model: path and name of the original network model file.
    - --framework: framework type of the original network model. "3" indicates a TensorFlow model, "5" indicates an ONNX model, and "1" indicates a model file in the MindSpore framework \*.air format or a standard \*.air format file exported by TorchAir.
    - --output: path and name for storing the converted offline model. For example, if it is set to `$HOME/module/out/tf_resnet50`, the converted offline model is stored in `$HOME/module/out/` and named “tf\_resnet50.om”.
    <!-- npu="950,A3,910b,910,310p,310b" id1 -->
    - --soc\_version: model of the AI processor. The value can be obtained as follows:
        <!-- npu="910b,910,310p,310b" id3 -->
        - For the following products: run the `npu-smi info` command on the server where the AI processor is installed to query the **Name** information. The actual configuration value is AscendName. For example, if the value of **Name** is `xxxyy`, the actual configuration value is `Ascendxxxyy`.

            Atlas A2 products

            Atlas 200I/500 A2 inference products

            Atlas inference products

            Atlas training products
        <!-- end id3 -->
        <!-- npu="A3" id2 -->
        - For Atlas A3 products, run the `npu-smi info -t board -i id -c chip_id` command on the server where the AI processor is installed to query and obtain the **Chip Name** and **NPU Name** information. The actual configuration value is the Chip Name\_NPU Name. For example, if the **Chip Name** value is `Ascendxxx` and the **NPU Name** value is 1234, the actual configuration value is `Ascendxxx_1234`. Where:
            - id: device ID. The NPU ID obtained by running the **npu-smi info -l** command is the device ID.
            - chip\_id: chip ID. The Chip ID obtained by running the **npu-smi info -m** command is the chip ID.
        <!-- end id2 -->
        <!-- npu="950" id4 -->
        - For Ascend 950PR&950DT products, run the `npu-smi info -t board -i id` command on the server where the AI processor is installed to query and obtain the **Chip Name** and **NPU Name** information. The actual configuration value is the Chip Name\_NPU Name. For example, if the **Chip Name** value is `Ascendxxx` and the **NPU Name** value is 1234, the actual configuration value is `Ascendxxx_1234`.

            Where id is the device ID. The NPU ID obtained by running the **npu-smi info -l** command is the device ID.
        <!-- end id4 -->
    <!-- end id1 -->

    <!-- @ref: ge/res/docs/zh/user_guides/graph_dev/quick_start_res.md#id1 -->

    For more parameters and detailed usage, see [Ascend Tensor Compiler (ATC)](../../atc_tools/README.md).

3. If the following information is displayed, the model conversion is successful.

   ```bash
   ATC run success, welcome to the next use.
   ```

   After the command is successfully executed, the \*.onnx model is converted into an \*.om offline model adapted to the AI processor. You can view the generated \*.om offline model file in the path specified by the --output parameter.

4. Follow-up operations.

   After the model is converted into an \*.om offline model, you can call the model loading API to load the converted \*.om offline model, and then call the model execution API for inference. The following is a brief process of loading a model file and performing inference. For details about model loading and inference, see the "Model Inference" section in *[Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md)*.

  ```c++
  // 1. Specify the model path.
  const char* omModelPath = "./model/onnx_resnet50.om";
  uint32_t modelId;
  // 2. Load the offline model file. If the model is loaded successfully, the ID that identifies the model is returned.
  ret = aclmdlLoadFromFile(omModelPath, &modelId);
  // 3. Execute the model. inputDs and outputDs are the Dataset objects for model input and output.
  ret = aclmdlExecute(modelId, inputDs, outputDs);
  // 4. Unload the model.
  ret = aclmdlUnload(modelId);
  ```

## Building a Graph from Scratch Using APIs

The C++ graph engine APIs of GE provide two ways to build a graph:

- **Build a graph from scratch using graph engine APIs**: You use the graph engine APIs to add operators to the compute graph one by one, building a compute graph represented in Ascend IR.
- **Parse an original model into a graph using the Parser API**: You use the Parser API to parse a framework model file (in formats such as \*.onnx and \*.pb) into a compute graph represented in Ascend IR.

This section combines the two scenarios above to show you how to build a graph. After building the graph, you can modify it as needed and then compile it into an offline model adapted to the AI processor. The overall process is as follows:

![](../figures/overall_process.png)

The process is described as follows:

1. Before building a graph, set up the environment, install the corresponding CANN software package, and analyze the network structure. If there are unsupported operators, develop the corresponding operators and deploy them to the hardware environment.
    1. If you **build a graph from scratch using graph engine APIs**, determine the following information based on the original network:
        1. Which operators are included in the network, as well as the inputs, outputs, attributes, and other information of these operators.
        2. The association relationships between operators in the network.
        3. Confirm whether the operators in the original network are supported by the AI processor. For the currently supported operators, see the "Ascend IR Operator Specification" section in [*Operator Library*](https://hiascend.com/document/redirect/CannCommunityOplist). If an operator is not supported or does not meet actual requirements, developers can customize an Ascend C operator by referring to [*Ascend C Operator Development*](https://gitcode.com/cann/asc-devkit/blob/9.2.0/docs/zh/guide/index.md), or customize a TBE operator by referring to [*TBE & AI CPU Operator Development*](https://hiascend.com/document/redirect/CannCommunityOpdevWizard). After selecting one approach, deploy the operator to the hardware environment.

    2. If **using the Parser API to parse the original model into a Graph**, confirm whether the operators in the original network are supported by the AI processor. For the currently supported operators, see the "Ascend IR Operator Specification" section in [*Operator Library*](https://hiascend.com/document/redirect/CannCommunityOplist). If an operator is not supported or does not meet actual requirements, you can customize an Ascend C operator by referring to [*Ascend C Operator Development*](https://gitcode.com/cann/asc-devkit/blob/9.2.0/docs/zh/guide/index.md), or customize a TBE operator by referring to [*TBE & AI CPU Operator Development*](https://hiascend.com/document/redirect/CannCommunityOpdevWizard). After selecting one approach, deploy the operator to the hardware environment.

2. Build a Graph. Developers can build a Graph from scratch using the graph engine APIs, or parse the original model into a Graph using the Parser API.
3. Modify the Graph. If developers want to optimize the Graph structure, they can directly modify the built Graph into the desired structure.
4. Compile and run the Graph. Developers can compile the modified Graph into an offline model adapted to the AI processor, and then load it through the model loading API for on-board inference. Alternatively, they can directly run the Graph to obtain the Graph execution result.

### Obtaining the Sample

Click [Build a Graph Using APIs](https://gitee.com/ascend/samples/tree/master/cplusplus/level1_single_api/3_ir/IRBuild) to obtain the sample code. The directory structure is as follows:

```text
├── src
│   ├──main.cpp                // Implementation file
├── Makefile                    // Compilation script
├── CMakeLists.txt              // Compilation script
├── data
│   ├──data_generate.py        // Generates the data information required by the Graph, such as weights and biases, when building a Graph from scratch using the graph engine APIs
│   ├──tensorflow_generate.py  // Generates a TensorFlow model in .pb format when parsing the original TensorFlow model into a Graph using the Parser APIs
│   ├──caffe_generate.py       // Generates a Caffe model in .pbtxt format and a weight file in .caffemodel format when parsing the original Caffe model into a Graph using the Parser APIs
├── scripts
│   ├──host_version.conf       // Version configuration
│   ├──testcase_300.sh         // Test script
```

This sample mainly includes the following functions:

1. Build a Graph.
    - **Build a Graph from scratch using graph engine APIs**

        The Graph structure constructed in this sample is as follows:

        ![Graph structure](../figures/ir_build_image1.png)

    - **Parse the original model into a Graph using the Parser API**. TensorFlow is used as an example here.

        The structure of the TensorFlow original model provided in this sample is as follows:

        ![TensorFlow original model structure](../figures/ir_build_image2.png)

        The structure of the parsed graph is as follows:

        ![Parsed graph structure](../figures/ir_build_image3.png)

2. Modify the Graph: This example uses the Graph obtained by parsing a TensorFlow model through the Parser API.

    The modified Graph structure is as follows, with an Abs operator inserted between the Add operator and the Const operator:

    ![Inserting the Abs operator](../figures/ir_build_image4.png)

3. Compile the Graph: Compile the final Graph above into an offline model.

### Compiling and Running

1. Prepare the environment.
    - See [Environment Preparation](environment_setup.md) to set up the environment and configure environment variables.
    - To parse a TensorFlow framework original model into a Graph, set up the TensorFlow 1.15.0 environment. The following is an example of installing it as the root user on the x86 architecture:

        ```python
        pip3 install tensorflow-cpu==1.15
        ```

        If you install it as a non-root user, add the --user parameter to the end of the preceding command. For the aarch64 architecture, see *[TensorFlow 1.15 Model Migration](https://hiascend.com/document/redirect/tfcommunityguide)* \> to install the open-source framework TensorFlow 1.15.

2. Prepare the graph construction data.
    - **Build a Graph from scratch using the graph engine APIs**.

        Run the data generation script in the data directory. The command is as follows:

        ```python
        python3 data_generate.py
        ```

        After the execution is complete, .bin data is generated in the data directory. During subsequent model construction, data such as weights and biases is read from this file.

    - **Parse the original model into a Graph using the Parser APIs**.

        Run the original model generation script in the data directory. The command is as follows:

        ```python
        python3 tensorflow_generate.py
        ```

        After the execution is complete, a model file in .pb format is generated in the data directory. This model is used later when parsing the original model, and its name is tf\_test.pb.

3. Compile the code.
    1. (Optional) Open the Makefile and modify the following **ASCEND\_PATH** path to the CANN software installation directory /cann. For example, for the root user, modify it to /usr/local/Ascend/cann.

        ```makefile
        ifeq (${ASCEND_INSTALL_PATH},)
             ASCEND_PATH := /usr/local/Ascend/cann
        ```

    2. Perform compilation.

        Run the following command in the same directory as the Makefile to compile:

        ```bash
        make clean && make ir_build
        ```

        After the compilation is complete, an executable file **ir\_build** is generated in the out folder under the current path.

4. Run the code.

    - **Build a Graph from scratch using the graph engine APIs**.

        Run the following command in the out folder:

        ```bash
        ./ir_build <soc_version> gen
        ```

        If the following information is displayed, the execution is successful, and the \*.om offline model file is generated in the same directory.

        ```text
        ========== Generate Graph1 Success!==========
        Build Model1 SUCCESS!
        Save Offline Model1 SUCCESS!
        ```

    - **Use the Parser API to parse the original model into a Graph**.

        Run the following command in the out folder:

        ```bash
        ./ir_build <soc_version> tf
        ```

        For how to query the value of <soc\_version\>, see [Converting an ONNX Model Using the ATC Command](#converting-an-onnx-model-using-atc).
        If the following information is displayed, the execution is successful, and the \*.om offline model file is generated in the same directory.

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

    After the model is converted into a \*.om offline model, you can call the model loading API to load the converted \*.om offline model, and then call the model execution API for inference. For details about model loading and inference, see "Model Inference" in *[Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md)*.

### Code Analysis

After experiencing the compilation and running of the quick start sample, if you want to understand the implementation logic of the code, you can refer to the code analysis below. The code logic is introduced here by the function of each function. To facilitate reading and understanding the code, the variables and functions related to each function are introduced together.

Open the "src/main.cpp" file, start with the main function to understand the code connection logic of the entire sample, and then understand the implementation of each custom function separately.

1. The main function is used to connect the code logic of the entire Graph construction.

    ```c++
    int main(int argc, char* argv[])
    {
        // 1. Construct the Graph.
        Graph graph1("IrGraph1");
           // 1.1 Use the graph engine interface to construct a Graph from scratch.
            ret = GenGraph(graph1);

           // 1.2 Use the Parser interface to parse the original model into a Graph.
            std::string tfPath = "../data/tf_test.pb";
            std::map<AscendString, AscendString> parser_options;
            tfStatus = ge::aclgrphParseTensorFlow(tfPath.c_str(), parser_options, graph1);

           // 2. Modify the Graph: Here, the Graph parsed from a TensorFlow model is used as an example for modification.
            ModifyGraph(graph1);

        // 3. Initialize the system and apply for resources.
        std::map<AscendString, AscendString> global_options = {
            {AscendString(ge::ir_option::SOC_VERSION), AscendString(argv[kSocVersion])}  ,
        };
        auto status = aclgrphBuildInitialize(global_options);

        // 4. Compile the Graph.
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

2. Include the required header files.

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

3. Build the Graph.

    Define operators by using the operator prototype derivation APIs and connect the operators. The detailed code example is as follows:

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
        // Call the API to set the input and output of the graph
        graph.SetInputs(inputs).SetOutputs(outputs);
        return true;
    }
    ```

4. Modify the graph: Here, the graph parsed from a TensorFlow model is used as an example for modification.

    Insert an Abs operator between the Add operator and the Const operator:

    ```c++
    bool ModifyGraph(Graph &graph) {

        const std::string CONST = "const1";
        const std::string ADD = "input3_add";
        GNode src_node;
        GNode dst_node;
        // Obtain all nodes in the graph.
        std::vector<GNode> nodes = graph.GetAllNodes();
        graphStatus ret = GRAPH_FAILED;
        for (auto &node : nodes) {
            ge::AscendString name;
            // Obtain the name of the operator in the graph.
            ret = node.GetName(name);

            std::string node_name(name.GetString());
        }
        // Delete the specified connection edge in the graph.
        ret = graph.RemoveEdge(src_node, 0, dst_node, 1);

        auto abs = op::Abs("input3_abs");

        // Add an abs node to the graph.
        GNode node_abs = graph.AddNodeByOp(abs);

       // Add a data edge to connect the newly added node.
        ret = graph.AddDataEdge(src_node, 0, node_abs, 0);

        ret = graph.AddDataEdge(node_abs, 0, dst_node, 1);
        return true;
    }
    ```

5. Compile the Graph.

    After the Graph is built and modified, call the API to compile the Graph. The API call process is shown in the following figure. For details, see the main\(\) function.

    ![Graph construction process](../figures/ir_graph_construction.png)
