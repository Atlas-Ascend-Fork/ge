# Dynamic Shape

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-21T09:59:15.338Z pushedAt=2026-10-28T08:35:37.436Z nl=2 -->

## Dynamic batch\_size

This section describes how to support the dynamic batch\_size feature during model construction.

### Feature Description

batch\_size refers to the number of images processed in each model inference. For scenarios where the number of images per inference is fixed, the number of processed images is determined by the N value of the data shape. For scenarios where the number of images per inference is not fixed, the dynamic batch\_size feature can be used to dynamically allocate the number of images processed each time. For example, if a user needs to process 2, 4, or 8 images each time when executing an inference service, the user can configure the gear information 2,4,8 in the model. After the gears are applied for, memory is allocated based on the actual gear during model inference.

### Usage Method

1. In the Data operator definition, set the specified dimension of the data shape to -1:

    ```c++
    auto shape_data = vector<int64_t>({ -1,1,28,28 });
    TensorDesc desc_data(ge::Shape(shape_data), FORMAT_NCHW, DT_FLOAT);
    auto data = op::Data("data");
    data.update_input_desc_data(desc_data);
    data.update_output_desc_out(desc_data);
    ```

2. During model compilation, set the INPUT\_SHAPE/INPUT\_FORMAT information in the options of the aclgrphBuildModel API, and specify the value of each gear through DYNAMIC\_BATCH\_SIZE.

    > [!NOTE]
    >
    >- INPUT\_FORMAT must be set and must be consistent with the format of all Data operators. Only NCHW and NHWC are supported. Otherwise, model compilation fails.
    >- The INPUT\_SHAPE parameter must be set.

    ```c++
    void PrepareOptions(std::map<AscendString, AscendString>& options) {
        options.insert({
            {ge::ir_option::INPUT_FORMAT, "NCHW"},
            {ge::ir_option::INPUT_SHAPE, "data:-1,1,28,28"}, // -1 in INPUT_SHAPE indicates setting a dynamic batch.
            {ge::ir_option::DYNAMIC_BATCH_SIZE, "2,4,8"}     // Set the gears of N.
        });
    }
    ```

### Usage Notes

- This feature cannot be used together with Dynamic Resolution or Dynamic Dimension.

- If the gear values set by the user are too large or there are too many gears, Model Compilation may fail. In this case, it is recommended that the user reduce the number of gears or lower the gear values.
- If the number of images processed each time is not fixed when the user performs inference, this parameter can be configured to dynamically allocate the number of images processed each time. For example, if the user needs to process 2, 4, and 8 images each time during inference, the parameter can be configured as 2,4,8. After the gears are applied for, memory is allocated based on the actual gear during model Inference.
- If dynamic batch\_size is set through this parameter during model compilation, when model inference is performed using the application project, before the **model execution** API:

  - Use the [aclmdlSetDynamicBatchSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicBatchSize.md) API to set the actual batch\_size gear.
  - If the [aclmdlSetDynamicBatchSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicBatchSize.md) API is not used, the maximum value in the range set by batch\_size is used by default during Model Execution.

- If the user sets dynamic batch\_size and also sets the dynamic AIPP feature through the **INSERT\_OP\_FILE** parameter:

    During actual inference, when calling the [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md) API to set the dynamic AIPP-related parameter values, ensure that batch\_size is set to the maximum batch number.

- In some scenarios, after the dynamic batch\_size feature is set through this parameter, the network structure of the generated offline model differs from that in the fixed batch\_size scenario, and inference performance may differ.
- If the gear values set by the user are too large or there are too many gears, when performing inference in the running environment, it is recommended to run the **swapoff -a** command to disable the use of swap space as memory, preventing the situation where the running environment becomes abnormally slow because swap space is continuously used as memory due to insufficient memory.

## Dynamic Resolution

This section describes how to support dynamic resolution during model construction.

### Feature Description

During model inference, for scenarios where the width and height of each processed image are not fixed, users can set different image width and height levels during model construction.

### Usage Method

1. When defining the Data operator, set both the H and W dimensions of the data shape to -1:

    ```c++
    auto shape_data = vector<int64_t>({ 8,3,-1,-1 });
    TensorDesc desc_data(ge::Shape(shape_data), FORMAT_NCHW, DT_FLOAT);
    auto data = op::Data("data");
    data.update_input_desc_data(desc_data);
    data.update_output_desc_out(desc_data);
    ```

2. At model compilation time, set the INPUT\_SHAPE/INPUT\_FORMAT information in the options of the aclgrphBuildModel API, and specify the dynamic resolution levels through DYNAMIC\_IMAGE\_SIZE.

    > [!NOTE]
    >
    >- INPUT\_FORMAT must be set and must be consistent with the format of all Data operators. Only NCHW and NHWC are supported. Otherwise, model compilation fails.
    >- The INPUT\_SHAPE parameter must be set.

    ```c++
    void PrepareOptions(std::map<AscendString, AscendString>& options) {
        options.insert({
            {ge::ir_option::INPUT_FORMAT, "NCHW"},
            {ge::ir_option::INPUT_SHAPE, "data: 8,3,-1,-1"},
            {ge::ir_option::DYNAMIC_IMAGE_SIZE, "416,416;832,832"}  // Set the HW levels. Images with HW of 416,416 or 832,832 are supported.
        });
    }
    ```

### Usage Notes

- This feature cannot be used together with Dynamic Batch or Dynamic Dimension.
- If the resolution values set by the user are too large or there are too many levels, model compilation may fail. In this case, the user is advised to reduce the number of levels or lower the level values.
- If Dynamic Resolution is set through this parameter during model compilation, when Model Inference is performed using the application project, before the **Model Execution** API:

  - Use the [aclmdlSetDynamicHWSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicHWSize.md) API to set the actual resolution. During actual inference, the size of the dataset images used must match the specific resolution used.
  - If the [aclmdlSetDynamicHWSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicHWSize.md) API is not used, the width and height of the maximum level in the Dynamic Resolution setting range are assigned by default during model execution.

- If the user sets Dynamic Resolution, that is, the width and height of the input image are uncertain, and also sets the static AIPP feature through the **INSERT\_OP\_FILE** parameter: in this scenario, the Crop and Padding features cannot be enabled in the AIPP configuration file, and the values of src\_image\_size\_w and src\_image\_size\_h in the configuration file must be set to 0.
- If the user sets Dynamic Resolution and also sets the dynamic AIPP feature through the **INSERT\_OP\_FILE** parameter:

    During actual inference, when calling the [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md) API to set the dynamic AIPP-related parameter values, the Crop and Padding features cannot be enabled. In this scenario, it is also necessary to ensure that the width and height set through the aclmdlSetInputAIPP API are equal to the width and height set through the [aclmdlSetDynamicHWSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicHWSize.md) API, and both must be set to the width and height of the maximum level of Dynamic Resolution.

- In certain scenarios, after the Dynamic Resolution feature is set through this parameter, the network structure of the generated offline model may differ from that in the fixed-resolution scenario, and inference performance may differ.
- If the user has set dynamic resolution, during actual inference, the size of the images in the dataset used must match the specific resolution in use.
- If the resolution values set by the user are too large or there are too many levels, when performing inference in the runtime environment, it is recommended to run the **swapoff -a** command to disable the use of swap space as memory, so as to prevent the runtime environment from becoming extremely slow due to insufficient memory that causes the swap space to be continuously used as memory.

## Dynamic Dimension

### Compiling a Graph into an Offline Model

The system supports configuring dynamic dimension information in APIs to support dynamic input scenarios. This section provides detailed instructions.

#### Feature Description

Users can set the Dynamic Dimension levels in ND format during Model Construction. This applies to scenarios where each inference processes dimensions of arbitrary size.

#### Usage Method

1. When defining the Data operator, set the dynamic dimension of the data shape to -1:

    ```c++
    auto shape_data = vector<int64_t>({ 1,-1,-1 });
    TensorDesc desc_data(ge::Shape(shape_data), FORMAT_ND, DT_FLOAT);
    auto data = op::Data("data");
    data.update_input_desc_data(desc_data);
    data.update_output_desc_out(desc_data);
    ```

2. During model compilation, set the INPUT\_SHAPE/INPUT\_FORMAT information in the options of the aclgrphBuildModel API, and specify the gear information through DYNAMIC\_DIMS.

    > [!NOTE]
    >
    >- INPUT\_FORMAT must be set and must be consistent with the format of all Data operators. Only ND is supported. Otherwise, model compilation fails.
    >- The INPUT\_SHAPE parameter must be set.

    ```c++
    void PrepareOptions(std::map<std::string, std::string>& options) {
        options.insert({
            {ge::ir_option::INPUT_FORMAT, "ND"},
            {ge::ir_option::INPUT_SHAPE, "data:1,-1,-1"},
            {ge::ir_option::DYNAMIC_DIMS, "1,2;3,4;5,6;7,8"}  // During model compilation, the supported shapes of the data operator are 1,1,2; 1,3,4; 1,5,6; 1,7,8
        });
    }
    ```

#### Usage Notes

- This feature cannot be used together with dynamic batch\_size, dynamic resolution, or AIPP.
- Parameters are set in the format "dim1,dim2,dim3;dim4,dim5,dim6;dim7,dim8,dim9". All levels must be enclosed in double quotation marks, and each level is separated by an English semicolon. The dim values in each level correspond in sequence to the parameters identified by -1 in the INPUT\_SHAPE parameter. The number of dimensions that must be set in each level equals the number of -1 values in the INPUT\_SHAPE parameter. For example:

    ```c++
    void PrepareOptions(std::map<std::string, std::string>& options) {
        options.insert({
            {ge::ir_option::INPUT_FORMAT, "ND"},
            {ge::ir_option::INPUT_SHAPE, "data:1,1,40,-1;label:1,-1;mask:-1,-1"},
            {ge::ir_option::DYNAMIC_DIMS, "20,20,1,1;40,40,2,2;80,60,4,4"}
        });
    }
    ```

    In this case, during model compilation, the supported input shapes are:

    Level 0: data\(1,1,40,20\)+label\(1,20\)+mask\(1,1\)

    Level 1: data\(1,1,40,40\)+label\(1,40\)+mask\(2,2\)

    Level 2: data\(1,1,40,80\)+label\(1,60\)+mask\(4,4\)

- If dynamic dimensions are set through this parameter during model compilation, before the **Model Execution** API is called during model inference using the application project:

  - Use the [aclmdlSetInputDynamicDims](../../../api/graph_engine_api/c/acl/aclmdlSetInputDynamicDims.md) API to set the actual dimensions.
  - If the [aclmdlSetInputDynamicDims](../../../api/graph_engine_api/c/acl/aclmdlSetInputDynamicDims.md) API is not used, the model is executed with the maximum values in the dynamic dimension setting range by default.

### Graph Build and Run

The current system supports configuring dynamic dimension information in user scripts to support dynamic input scenarios. This section provides specific instructions.

#### Feature Description

<!-- npu="310b" id3 -->
Atlas 200I/500 A2 inference products do not support this feature.
<!-- end id3 -->

In the compile-and-run Graph scenario, Dynamic Dimension currently supports only whole-graph setting: use Session parameters to set the dimension information, and the input can be in dataset mode, placeholder mode, or a combination of both. For mixed input, only the scenario where one of them changes dynamically is currently supported.

#### Usage Method

1. When defining the Data operator, set the dynamic dimension of the data shape to -1:

    ```c++
    auto shape_data = vector<int64_t>({ 1,-1,-1 });
    TensorDesc desc_data(ge::Shape(shape_data), FORMAT_ND, DT_FLOAT);
    auto data = op::Data("data");
    data.update_input_desc_data(desc_data);
    data.update_output_desc_out(desc_data);
    ```

2. When compiling and running a graph, set the ge.inputShape/ge.dynamicNodeType information in the options of the Session API and the AddGraph API, and specify the dimension information through ge.dynamicDims.

    > [!NOTE]
    >
    >- The ge.inputShape parameter must be set.
    >- When setting dynamic dimensions for the entire graph, the input order of ge.inputShape set in the script must be consistent with the alphabetical order of the names of the actual data nodes. For example, if there are three inputs: label, data, and mask, the input order of ge.inputShape should be data, label, mask:
    >
    >    ```c++
    >    "data:1,1,40,-1;label:1,-1;mask:-1,-1"
    >    ```
    >
    >- In the dynamic dimension scenario of compiling and running a graph, if the model has multiple inputs, non-profiled inputs support specifying the internal Format, while profiled inputs do not support specifying the internal Format. For details about the usage, see [Specifying the Internal Formats of Graph Inputs and Outputs](specify_graph_io_format.md).

    ```c++
    std::map<std::string, std::string> options = {{"ge.inputShape", "data:1,1,40,-1;label:1,-1;mask:-1,-1"},
                                                 {"ge.dynamicDims", "20,20,1,1;40,40,2,2;80,60,4,4"},
                                                 {"ge.dynamicNodeType", "1"}};
    // Add dynamic dimension parameters to the Session
    ge::GeSession *session = new ge::GeSession(options);
    // Add dynamic dimension parameters to the graph
    session->AddGraph(graph_id, graph, options);
    ```

- ge.inputShape indicates the input shape information of the network. The preceding configuration indicates that the network has three inputs named data, label, and mask, with shapes (1,1,40,-1), (1,-1), and (-1,-1), respectively. Here, -1 indicates that the dimension is dynamic and must be set through ge.dynamicDims.
- ge.dynamicDims is used to set the profiles of dynamic dimensions. Profiles are separated by English semicolons. The dim values in each profile correspond in sequence to the parameters marked with -1 in the ge.inputShape parameter. The number of dimensions set in each profile must equal the number of -1 values in the ge.inputShape parameter. Combined with the ge.inputShape information, if ge.dynamicDims is configured as "20,20,1,1;40,40,2,2;80,60,4,4", the input shape supports three profiles. At model compilation time, the supported input combination profiles are as follows:
  - Profile 0: data\(1,1,40,20\), label\(1,20\), mask\(1,1\)
  - Profile 1: data\(1,1,40,40\), label\(1,40\), mask\(2,2\)
  - Tier 2: data\(1,1,40,80\), label\(1,60\), mask\(4,4\)

- ge.dynamicNodeType specifies the node type of the dynamic input. 0: dataset input is a dynamic input; 1: placeholder input is a dynamic input. Currently, dataset and placeholder inputs cannot both be dynamic inputs.

## Dynamic Input Shape Range

This section describes how to set the shape range of dynamic inputs during model construction.

### Feature Description

<!-- npu="310b" id4 -->
Atlas 200I/500 A2 inference products do not support this feature.
<!-- end id4 -->

During model compilation, users can specify the shape range of the model input data, so that a model supporting dynamic input can be compiled.

### Usage Method

1. When defining the Data operator, set the specified dimension of the data shape to -1:

    ```c++
    auto shape_data = vector<int64_t>({ -1,3,5,-1 });
    TensorDesc desc_data(ge::Shape(shape_data), FORMAT_NCHW, DT_FLOAT);
    auto data = op::Data("data");
    data.update_input_desc_x(desc_data);
    data.update_output_desc_y(desc_data);
    ```

2. During model compilation, specify the shape range of the model input data in the options of the aclgrphBuildModel API through INPUT\_SHAPE.

    ```c++
    void PrepareOptions(std::map<AscendString, AscendString>& options) {
        options.insert({
            {ge::ir_option::INPUT_FORMAT, "NCHW"},
            {ge::ir_option::INPUT_SHAPE, "8~20,3,5,-1"}
        });
    }
    ```

    Format requirements for setting the shape range through the INPUT\_SHAPE parameter:

    - Setting by name is supported: "input\_name1:n1,c1,h1,w1;input\_name2:n2,c2,h2,w2", for example: "input\_name1:8\~20,3,5,-1;input\_name2:5,3\~9,10,-1". The specified node must be enclosed in double quotation marks, and nodes are separated by English semicolons. input\_name must be the node name in the network model before conversion. If you know the name of the data node, setting by name is recommended.
    - Setting by index is supported: "n1,c1,h1,w1;n2,c2,h2,w2", for example: "8\~20,3,5,-1;5,3\~9,10,-1". The node name can be omitted, and nodes are arranged in index order and separated by English semicolons. When setting the shape range by index, the data node must have the index attribute set to indicate which input it is, with the index starting from 0.
    - For dynamic dimensions, a shape range is represented by a tilde “\~”; a fixed dimension is represented by a fixed number; and an unbounded range is represented by -1.

### Usage Notes

If the shape range of the dynamic input is set through this parameter during model compilation, when using the application project for model inference, call the [aclmdlSetDatasetTensorDesc](../../../api/graph_engine_api/c/acl/aclmdlSetDatasetTensorDesc.md) API before the model execution [aclmdlExecute](../../../api/graph_engine_api/c/acl/aclmdlExecute.md) API to set the actual input tensor description (input shape range). After model execution, call the [aclmdlGetDatasetTensorDesc](../../../api/graph_engine_api/c/acl/aclmdlGetDatasetTensorDesc.md) API to obtain the tensor description of the model's dynamic output, and then call the operation APIs under [aclTensorDesc](../../../api/graph_engine_api/c/acl/aclTensorDesc.md) to obtain the memory size occupied by the output tensor data, the Format information of the tensor, the dimension information of the tensor, and so on.
