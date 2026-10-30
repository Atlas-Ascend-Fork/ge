# Operator Prototype Definition

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-21T09:28:47.196Z pushedAt=2026-10-28T03:30:34.483Z nl=2 -->

Before adding operators one by one to a computation graph through the graph engine API to build a computation graph represented by Ascend IR, you need to understand the content of operator prototypes.

## Operator Prototype Definition

An operator prototype mainly describes the inputs, outputs, and attributes of an operator, as well as the related implementation information of the operator on the AI processor.

During operator development, use the REG\_OP macro to register the input, output, and attribute information of an operator by linking interfaces such as INPUT, OUTPUT, and ATTR with ".", and finally end with the OP\_END\_FACTORY\_REG interface to complete operator registration. The registration code is implemented as follows:

```c++
namespace ge{
    REG_OP(OpType) // Operator type name
    .INPUT(x1, TensorType({DT_FLOAT, DT_INT32}))
    .INPUT(x2, TensorType({DT_FLOAT, DT_INT32}))
    .DYNAMIC_INPUT(a, TensorType({DT_FLOAT, DT_INT32}))
    .OPTIONAL_INPUT(b, TensorType({DT_FLOAT}))
    .OUTPUT(y, TensorType({DT_FLOAT, DT_INT32}))
    .DYNAMIC_OUTPUT(c, TensorType({DT_FLOAT, DT_INT32}))
    .ATTR(x, Type, DefaultValue)
    .REQUIRED_ATTR(d, Type)
    .OP_END_FACTORY_REG(OpType)
}
```

- REG\_OP\(_OpType_\)

    OpType: operator type registered to the custom operator library of the AI processor.

- INPUT\(_x1,_  TensorType\(\{_DT\_FLOAT,DT\_UINT8,..._  \}\)\)

    Registers the input information of the operator.

  - x1: macro parameter, the input name of the operator.
  - TensorType\(\{_DT\_FLOAT,DT\_UINT8,..._  \}\): “\{ \}” contains the list of data types supported by this input.

    If an operator has multiple inputs, each input needs to be described using one INPUT\(_x,_  TensorType\(\{_DT\_FLOAT,DT\_UINT8,..._  \}\)\) statement.

- DYNAMIC\_INPUT\(_a_, TensorType\(\{_DT\_FLOAT, DT\_INT32_,... \}\)\)

    Registers input information for an operator in a dynamic multi-input scenario.

  - a: macro parameter, the name of the operator input. During graph execution, a0, a1, a2, and so on are automatically generated based on the number of inputs, with sequence numbers increasing in order.
  - TensorType\(\{_DT\_FLOAT, DT\_INT32_,... \}\): “\{ \}” contains the list of data types supported by this input.

- OPTIONAL\_INPUT\(_b_, TensorType\(\{_DT\_FLOAT,..._\}\)\)

    If an operator input is optional, this interface can be used to register the operator input.

  - b: macro parameter, the name of the operator input.
  - TensorType\(\{_DT\_FLOAT,..._\}\): “\{ \}” contains the list of data types supported by this input.

- OUTPUT\(_y_, TensorType\(\{_DT\_FLOAT,DT\_UINT8,..._  \}\)\)

    Register the output information of the operator.

  - y: macro parameter, the output name of the operator.
  - TensorType\(\{_DT\_FLOAT,DT\_UINT8,..._  \}\): “\{ \}” contains the list of data types supported by this output.

    If an operator has multiple outputs, each output must be registered using a separate OUTPUT\(_y,_  TensorType\(\{  _DT\_FLOAT,DT\_UINT8,..._  \}\)\) statement.

- DYNAMIC\_OUTPUT\(_c_, TensorType\(\{_DT\_FLOAT, DT\_INT32,..._\}\)\)

    Registers the output information of an operator in the dynamic multi-output scenario.

  - c: macro parameter, the output name of the operator. During graph execution, c0, c1, c2, and so on are automatically generated based on the number of outputs, with the sequence number increasing in order.
  - TensorType\(\{_DT\_FLOAT,DT\_UINT32,..._  \}\): “\{ \}” contains the list of data types supported by this output.

- ATTR\(_x_, Type, _DefaultValue_\)

    Registers the attributes of the operator, including the attribute name, attribute type, and default value of the attribute. When the developer does not set the attribute value of the operator object, the system uses the default value.

    For example, for ATTR\(mode, Int, 1\), register the attribute mode, whose attribute type is int64\_t and default value is **1**.

    If an operator has multiple attributes, each attribute must be registered with an ATTR\(_x_, Type, _DefaultValue_\) statement.

- REQUIRED\_ATTR\(_d_, Type\)

    Register the attributes of the operator, including the attribute name and attribute type, without a default value. Developers must set the attribute value of the operator object.

    If an operator has multiple attributes, each attribute must be registered with a REQUIRED\_ATTR\(_d_, Type\) statement.

- OP\_END\_FACTORY\_REG\(_OpType_\): End operator registration. OpType must be consistent with the OpType in REG\_OP\(_OpType_\).

>[!NOTE]Note
>
>For the mapping between DT\_FLOAT, DT\_UINT8_, and other data types, see \>ge Namespace \>DataType in [Basic Data Structures and APIs](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md).

## How to Obtain an Operator Prototype

When building a model, you need to understand the operator prototype, including the input, output, and attribute information, so that you can create operator instances and build your own Graph.

- For user-defined operators, obtain the header file that defines the operator prototype of the custom operator and understand its prototype definition.

    Obtain it from the operator prototype definition header file in `${INSTALL_DIR}/opp/vendors/<vendor_name>/op_proto/inc`, for example:

    ```c++
    REG_OP(SubMConv3dCube)
        .INPUT(x, TensorType({DT_FLOAT16}))
        .INPUT(filter, TensorType({DT_FLOAT16}))
        .OUTPUT(y, TensorType({DT_FLOAT16}))
        .ATTR(is_first, Bool, false)
        .OP_END_FACTORY_REG(SubMConv3dCube)
    ```

- For built-in operators, you can obtain the operator prototype in either of the following two ways:

  - Obtain it from "Ascend IR Operator Specification" in the [Operator Library](https://hiascend.com/document/redirect/CannCommunityOplist), as shown in the following figure.

      **Figure 1**  View operator prototype information
      ![Operator prototype](../figures/view_op_prototype_info.png "View operator prototype")

  - Obtain it from the operator prototype definition header file in `${INSTALL_DIR}/opp/built-in/op_graph/inc`, for example:

      ```c++
      REG_OP(SoftmaxV2)
          .INPUT(x, TensorType({DT_DOUBLE, DT_FLOAT16, DT_BF16, DT_FLOAT}))
          .OUTPUT(y, TensorType({DT_DOUBLE, DT_FLOAT16, DT_BF16, DT_FLOAT}))
          .ATTR(axes, ListInt, {-1})
          .ATTR(half_to_float, Bool, false)
          .OP_END_FACTORY_REG(SoftmaxV2)
      ```

    Here, $\{INSTALL\_DIR\} must be replaced with the file storage path after the CANN software is installed. For example, if the software is installed by the root user, the default storage path after installation is /usr/local/Ascend/cann.
