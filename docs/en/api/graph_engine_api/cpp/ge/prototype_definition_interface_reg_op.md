# Prototype Definition APIs (REG\_OP)

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T11:27:03.604Z pushedAt=2026-10-28T07:40:22.181Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

Defines the operator prototype, including the operator's inputs, outputs, attributes, and corresponding data types.

After the operator prototype is defined as above, it is equivalent to registering the operator prototype with GE, informing GE of the inputs, outputs, and attributes that the corresponding type of operator should have. At the same time, it defines a class named op::xxx. Developers can include the prototype header file and then instantiate the class to build an IR model, as shown below:

```c++
conv = op::Conv2D()
conv.set_input_x(feature_map_data)
conv.set_input_filter(weight_data)
```

For details about model building, see *Graph Development*.

## Prototype

The following is an example of a prototype definition:

```c++
REG_OP(xxx)
    .INPUT(x1, type)
    .OPTIONAL_INPUT(x2, type)
    .DYNAMIC_INPUT(x3, type)
    .OUTPUT(y1, type)
    .DYNAMIC_OUTPUT(y3, type)
    .REQUIRED_ATTR(a, type)
    .ATTR(b, type, default_value)
    .GRAPH(z1)
    .DYNAMIC_GRAPH(z2)
    .OP_END_FACTORY_REG(xxx)
```

## API Description

| Name | Description | Derived Interface (for IR model construction) |
| --- | --- | --- |
| **REG_OP(xxx)** | Defines an operator prototype, with the operator type being xxx. | [REG_OP](./prototype_definition_derived_interface.md#reg_op) |
| **.INPUT(x, type)** | Defines the input name (x) and type (type).<br>The type is TensorType, for example:<br>  - **TensorType{DT_FLOAT}**<br>  - **TensorType({DT_FLOAT, DT_INT8})**<br>  - **TensorType::ALL()**<br><br><br>For information about the TensorType class, see also [TensorType Class Description](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). | [INPUT](./prototype_definition_derived_interface.md#input) |
| **.OPTIONAL_INPUT(x, type)** | Defines the name (x) and type (type) of an optional input.<br>The type is TensorType, for example:<br>  - **TensorType{DT_FLOAT}**<br>  - **TensorType({DT_FLOAT, DT_INT8})**<br>  - **TensorType::ALL()**<br><br><br>For information about the TensorType class, see also [TensorType Class Description](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). | [OPTIONAL_INPUT](./prototype_definition_derived_interface.md#optional_input) |
| **.DYNAMIC_INPUT(x, type)** | Defines the name (x) and type (type) of a dynamic input.<br>The type is TensorType, for example:<br>  - **TensorType{DT_FLOAT}**<br>  - **TensorType({DT_FLOAT, DT_INT8})**<br>  - **TensorType::ALL()**<br><br><br>For information about the TensorType class, see also [TensorType Class Description](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). | [DYNAMIC_INPUT](./prototype_definition_derived_interface.md#dynamic_input) |
| **.OUTPUT(x, type)** | Defines the name (x) and type (type) of an output.<br>The type is TensorType, for example:<br>  - **TensorType{DT_FLOAT}**<br>  - **TensorType({DT_FLOAT, DT_INT8})**<br>  - **TensorType::ALL()**<br><br><br>For information about the TensorType class, see also [TensorType Class Description](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). | [OUTPUT](./prototype_definition_derived_interface.md#output) |
| **.DYNAMIC_OUTPUT(x, type)** | Defines the name (x) and type (type) of a dynamic output.<br>The type is TensorType, for example:<br>  - **TensorType{DT_FLOAT}**<br>  - **TensorType({DT_FLOAT, DT_INT8})**<br>  - **TensorType::ALL()**<br><br><br>For information about the TensorType class, see also [TensorType Class Description](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). | [DYNAMIC_OUTPUT](./prototype_definition_derived_interface.md#dynamic_output) |
| **.REQUIRED_ATTR(x, type)** | Defines the name (x) and type (type) of a required attribute.<br>The optional values of type include:<br>  - Int, the attribute type is int64_t<br>  - Float, the attribute type is float<br>  - String, the attribute type is string<br>  - Bool, the attribute type is bool<br>  - Tensor, the attribute type is Tensor<br>  - Type, the attribute is a Type enumeration definition<br>  - NamedAttrs, the attribute type is NamedAttrs<br>  - AscendString, the attribute type is AscendString<br>  - ListInt, the attribute type is vector<int64_t>, an int64_t list<br>  - ListFloat, the attribute type is vector\<float>, a float list<br>  - ListString, the attribute type is vector\<string>, a string list<br>  - ListBool, the attribute type is vector\<bool>, a bool list<br>  - ListTensor, the attribute type is vector\<Tensor>, a Tensor list<br>  - Bytes, the attribute type is Buffer<br>  - ListType, the attribute type is vector\<Type>, a Type list<br>  - ListListInt, the attribute type is vector<vector<int64_t>>, a 2D list<br>  - ListAscendString, the attribute type is vector\<AscendString>, an AscendString list<br>  - ListNamedAttrs, the attribute type is vector\<NamedAttrs>, a NamedAttrs list | [REQUIRED_ATTR](./prototype_definition_derived_interface.md#required_attr) |
| **.ATTR(x, type, default_value)** | Defines the name, type, and default value of an optional attribute.<br>When the user does not set the attribute of the operator object, the default value set here is used.<br>The optional values of type include:<br>  - Int, the attribute type is int64_t<br>  - Float, the attribute type is float<br>  - String, the attribute type is string<br>  - Bool, the attribute type is bool<br>  - Tensor, the attribute type is Tensor<br>  - Type, the attribute is a Type enumeration definition<br>  - NamedAttrs, the attribute type is NamedAttrs<br>  - AscendString, the attribute type is AscendString<br>  - ListInt, the attribute type is vector<int64_t>, an int64_t list<br>  - ListFloat, the attribute type is vector\<float>, a float list<br>  - ListString, the attribute type is vector\<string>, a string list<br>  - ListBool, the attribute type is vector\<bool>, a bool list<br>  - ListTensor, the attribute type is vector\<Tensor>, a Tensor list<br>  - Bytes, the attribute type is Buffer<br>  - ListType, the attribute type is vector\<Type>, a Type list<br>  - ListListInt, the attribute type is vector<vector<int64_t>>, a 2D list<br>  - ListAscendString, the attribute type is vector\<AscendString>, an AscendString list<br>  - ListNamedAttrs, the attribute type is vector\<NamedAttrs>, a NamedAttrs list<br><br><br>Definition example:<br><br>  - .ATTR(mode, Int, 1)<br>  - .ATTR(pad, ListInt, {0, 0, 0, 0}) | [ATTR](./prototype_definition_derived_interface.md#attr) |
| **.GRAPH(z1)** | Registers the subgraph information contained in the operator. The input z1 is the subgraph name.<br>For example, the subgraphs registered for the If operator are:<br>.GRAPH(then_branch)  .GRAPH(else_branch)<br>For the same operator, the registered operator subgraph names must be unique. | [GRAPH](./prototype_definition_derived_interface.md#graph) |
| **.DYNAMIC_GRAPH(z2)** | Registers the dynamic operator subgraph information. The input z2 is the subgraph name.<br>For example, the subgraph registered for the Case operator is:<br>.DYNAMIC_GRAPH(branches)<br>For the same operator, the registered operator subgraph names must be unique. | [DYNAMIC_GRAPH](./prototype_definition_derived_interface.md#dynamic_graph) |
| **.INFER_SHAPE_AND_TYPE()** | This API is a legacy compatibility API and does not need to be used by users in the current version. | - |
| **.OP_END_FACTORY_REG(x)** | Pairs with REG_OP to end the operator prototype definition.<br>The operator type (x) is the same as the type in REG_OP(x). | - |

> [!NOTE]
> The function of the **OpReg &N\(\)** API in the **OpReg** class is to allow users to call the interfaces of the **OpReg** class in the .\*\* manner during operator registration, for example, **.INPUT\(x, type\)** and **.OUTPUT\(x, type\)**. It has no other meaning.

## Return Value

None

## Constraints

- The operator type of **REG\_OP** must be globally unique.
- Input names of the same operator must not be duplicated.
- Output names of the same operator must not be duplicated.
- Attribute names of the same operator must not be duplicated.

## Examples and Related APIs

Operator prototype definition example for dynamic input:

```c++
REG_OP(AddN)
    .DYNAMIC_INPUT(x, TensorType({NumberType(), DT_VARIANT}))
    .OUTPUT(y, TensorType({NumberType(), DT_VARIANT}))
    .REQUIRED_ATTR(N, Int)
    .OP_END_FACTORY_REG(AddN)
```

Operator prototype definition example for multi-input:

```c++
REG_OP(GreaterEqual)
    .INPUT(x1, TensorType::RealNumberType())
    .INPUT(x2, TensorType::RealNumberType())
    .OUTPUT(y, TensorType({DT_BOOL}))
    .OP_END_FACTORY_REG(GreaterEqual)
```

Operator prototype definition example for registering a subgraph:

```c++
REG_OP(If)
    .INPUT(cond, TensorType::ALL())
    .DYNAMIC_INPUT(input, TensorType::ALL())
    .DYNAMIC_OUTPUT(output, TensorType::ALL())
    .GRAPH(then_branch)
    .GRAPH(else_branch)
    .OP_END_FACTORY_REG(If)
```
