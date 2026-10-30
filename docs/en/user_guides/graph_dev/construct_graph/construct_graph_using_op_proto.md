# Graph Construction from Operator Prototypes

<!-- md-trans-meta sourceCommit=1c12eaba19bc6be2051c1dfcd1db0c4e1089c516 translatedAt=2026-10-28T12:36:41.415Z pushedAt=2026-10-29T02:10:23.049Z nl=2 -->

This section describes how to build a complete Graph step by step using operator prototypes and graph engine APIs.

## Feature Overview

After an operator prototype is successfully registered using the REG\_OP macro, the corresponding derived APIs are automatically generated (see [Prototype Definition Derived APIs](../../../api/graph_engine_api/cpp/ge/prototype_definition_derived_interface.md)). You can use these APIs to define operators in a Graph, then create a Graph instance and set the input operators and output operators in the Graph to complete Graph construction.

![Illustration](../figures/feature_intro.png)

## Defining Operators Using Operator Prototype Derived APIs

This section describes only the general process of defining operators using operator prototype derived APIs. For definition examples of specific operator types (such as data nodes and compute nodes), see [Expression Examples of  Operators in a Graph](op_expr_samples_in_graph.md).

1. Include the header files.

    ```c++
    #include "ops_proto_math.h"
    #include "ops_proto_cv.h"
    #include "ops_proto_nn.h"
    #include "ops_proto_transformer.h"
    #include "ops_proto_legacy.h"
    ```

    - For built-in operators, to define built-in operator types, you can use the APIs related to built-in operator types. The header files are located in `${INSTALL_DIR}/opp/built-in/op_graph/inc/`.

        During development, include the header files of the corresponding operators as needed and include the legacy header file. For example, if you use the SoftmaxV2 operator, include both ops_proto_nn.h and ops_proto_legacy.h.
        Note: The legacy header file is not recommended. It is retained only for compatibility with some operators to be deprecated. When it is included together with other header files, it must be included last.

        ```c++
        #include "ops_proto_nn.h"
        #include "ops_proto_legacy.h"
        ```

    - For custom operators, you need to include the prototype definition header file of the custom operator. The header files are located in `${INSTALL_DIR}/opp/vendors/<vendor_name>/op_proto/inc`.

    In the path, $\{INSTALL\_DIR\} must be replaced with the file storage path after the CANN software is installed. For example, if you install the software as the root user, the default storage path is /usr/local/Ascend/cann.

2. Create an operator instance.

    After registering the operator type using the REG\_OP macro, the operator type constructor explicit OpType\(const char\* name\) is automatically generated, which is equivalent to defining a class named op::xxx. Developers include the prototype header file and instantiate the class to build a graph. When building a graph, the operator name can be directly passed as an input parameter, for example:

    ```c++
    auto softmax = op::SoftmaxV2("softmax")
    ```

    Note: The operator names in a graph **must be unique**.

3. Set the operator input.

    The operator prototype defines the input name, input type, and supported data types of the operator. Based on the input type, operator inputs can be classified into optional inputs, required inputs, and dynamic inputs.

    Optional inputs, required inputs, and dynamic inputs need to be set through different APIs.

    - Set the **required inputs and optional inputs** of the operator: Set them through “set\_input\__ input name_", for example:

        ```c++
        auto softmax = op::SoftmaxV2("softmax")         // Create a SoftmaxV2 operator instance
          .set_input_x(bias_add_3);                     // Set the SoftmaxV2 operator input to bias_add_3
        ```

    - Set the operator **dynamic inputs**: Create dynamic inputs through “create\_dynamic\_input\__ input name_" and set dynamic inputs through “set\_dynamic\_input\__ input name_". For an example, see [Defining a Dynamic Multi-Input Operator (AddN)](./op_expr_samples_in_graph.md#defining-a-dynamic-multi-input-operator-addn).

4. Set the operator attributes.

    The operator prototype defines the attribute names, attribute types, supported data types, default values, and value ranges of the operator attributes. Based on the attribute type, operator attributes are classified into required attributes (REQUIRED\_ATTR, whose values must be set when defining the operator) and optional attributes (ATTR, whose default values are used when no attribute value is set for the operator object).

    For both **required and optional attributes**, you can set them through the “set\_attr\__attribute name_" API, for example:

    ```c++
    auto maxpool1 = op::MaxPool("MaxPool1")
    .set_input_x(tanh1)
    .set_attr_ksize({1, 1, 2, 1})                // Set the ksize attribute value
    .set_attr_strides({1, 1, 2, 1})              // Set the strides attribute value
    .set_attr_padding("SAME");                   // Set the padding attribute value
    ```

## Operator Connection Edge Expression

Edges between operators are classified into data edges and control edges. A data edge specifies the input of an operator, and a control edge controls the execution order of operators.

1. Data edge expression.

    If the preceding operator has only one output, you can pass the name of the preceding operator through the “set\_input\_input name" API.

    ![Data edge expression 1](../figures/data_edge_expression_1.png)

    ```c++
    auto bias_add_3 = op::BiasAdd("bias_add_3")
      .set_input_x(matmul_2)
      .set_input_bias(bias_add_const_3)
      .set_attr_data_format("NCHW");
    auto softmax = op::SoftmaxV2("Softmax")
      .set_input_x(bias_add_3);
    ```

    If the preceding operator has multiple outputs, you need to pass the name and output name of the preceding operator, or pass the name and output index of the preceding operator.

    ![Data edge expression 2](../figures/data_edge_expression_2.png)

    Pass the name and output name of the preceding operator:

    ```c++
    auto data = op::Data("data");
    auto unique= op::Unique("unique").set_input_x(data);
    auto softplus = op::Softplus("softplus").set_input_x(unique, "y");  // Create the softplus operator and set its input to the y output of the unique operator.
    auto sqrt = op::Sqrt("sqrt").set_input_x(unique, "idx");            // Create the sqrt operator and set its input to the idx output of the unique operator.
    ```

    Pass the name and output index of the previous operator:

    ```c++
    auto data = op::Data("data");
    auto unique= op::Unique("unique").set_input_x(data);
    auto softplus = op::Softplus("softplus").set_input_x(unique, 0);  // Create the softplus operator and set its input to the first output of the unique operator
    auto sqrt = op::Sqrt("sqrt").set_input_x(unique, 1);              // Create the sqrt operator and set its input to the second output of the unique operator
    ```

2. Control edge expression.

    If the execution of an operator in the graph depends on the completion of other operators in the graph, as shown in the following figure, and you need to control the execution of Sqrt before Softplus, call the [AddControlInput](../../../api/graph_engine_api/cpp/ge/Operator/AddControlInput.md) API to add a control edge to the Softplus operator.

    ![Control edge expression](../figures/control_edge_expression.png)

    Example code:

    ```c++
    auto data = op::Data("data");
    auto unique= op::Unique("unique").set_input_x(data);
    auto sqrt = op::Sqrt("sqrt").set_input_x(unique, "idx");
    auto softplus = op::Softplus("softplus").set_input_x(unique, "y").AddControlInput(sqrt);
    ```

## Creating a Graph Instance

After the operators are defined, you need to create a Graph instance and set the input and output operators in the Graph. The main process is as follows:

1. Include the required header files.

    ```c++
    #include "graph.h"
    ```

2. Create a Graph object.

    ```c++
    Graph graph("IrGraph");
    ```

    For details about the related APIs, see [Graph](../../../api/graph_engine_api/cpp/ge/Graph/overview.md).

3. Set the input and output operators of the Graph. The main APIs used are as follows:

    - Set the input operator in the Graph: [SetInputs](../../../api/graph_engine_api/cpp/ge/Graph/SetInputs.md)
    - Set the output operator in the Graph: [SetOutputs](../../../api/graph_engine_api/cpp/ge/Graph/SetOutputs.md)

    For example, set the input of the Graph to the Data operator and the output to the Softmax operator:

    ```c++
    std::vector<Operator> inputs{data};      // data is an operator object of the Data type
    std::vector<Operator> outputs{softmax};  // softmax is an operator object of the Softmax type
    graph.SetInputs(inputs).SetOutputs(outputs);
    ```

    If the input consists of multiple Data operators, ensure that the order of the inputs parameter is consistent with the order specified by the index attribute of the Data operators. Otherwise, an error will be reported when the model is generated later. For example:

    ```c++
    // Prepare the first input data
    auto shape_data0 = vector<int64_t>({1,17,2,2});
    TensorDesc desc_data0(ge::Shape(shape_data0), FORMAT_ND, DT_FLOAT);
    auto data0 = op::Data("data0").set_attr_index(0) ;   // Create the data0 operator with the index attribute set to 0
    data0.update_input_desc_x(desc_data0);               // Set the input description of the operator
    data0.update_output_desc_y(desc_data0);              // Set the output description of the operator
    // Prepare the second input data
    auto shape_data1 = vector<int64_t>({1,5,2,2});
    TensorDesc desc_data1(ge::Shape(shape_data1), FORMAT_ND, DT_FLOAT);
    auto data1 = op::Data("data1").set_attr_index(1) ;   // Create the data1 operator with the index attribute set to 1
    data1.update_input_desc_x(desc_data1);               // Set the input description of the operator
    data1.update_output_desc_y(desc_data1);              // Set the output description of the operator

    // Set the Graph input operator.
    std::vector<Operator> inputs{data0, data1};
    ```

    >[!NOTE]Note
    >
    >During graph construction, if the number of shape dimensions differs from the number of format dimensions of a Tensor, interpret the current dimensions according to the rules in the following table:
    >For example, if the shape has only one dimension \[16\] and the format has four dimensions, such as NHWC, the one dimension of the shape is interpreted as the C axis, and the other axes need to be padded. After padding, the format is \[1,1,1,16\];
    >if the shape has two dimensions \[16,16\] and the format has four dimensions, such as NHWC, the two dimensions of the shape are interpreted as the HW axes, and the other axes need to be padded. After padding, the format is \[1,16,16,1\].
    > <!-- npu="950" id1 -->
    >This note **does not apply to** the Ascend 950PR&950DT products.
    > <!-- end id1 -->

|actual dimension count|format|dimension interpretation|
|--|--|--|
|1|NCHWNHWCHWCNCHWNNDHWCNCDHWDHWCNDHWNC|C|
|2|NCHW|CH|
|2|NHWC|HW|
|2|HWCN|CN|
|2|CHWN|WN|
|2|NDHWC|WC|
|2|NCDHW|HW|
|2|DHWCN|CN|
|2|DHWNC|NC|
|3|NCHW|CHW|
|3|NHWC|HWC|
|3|HWCN|WCN|
|3|CHWN|HWN|
|3|NDHWC|HWC|
|3|NCDHW|DHW|
|3|DHWCN|WCN|
|3|DHWNC|WNC|
|4|NDHWC|DHWC|
|4|NCDHW|CDHW|
|4|DHWCN|HWCN|
|4|DHWNC|HWNC|
