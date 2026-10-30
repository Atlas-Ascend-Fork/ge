# aclopCompile

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:43:25.110Z pushedAt=2026-10-29T03:42:28.646Z nl=2 -->

## Product Support

<!-- npu="950" id1182 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id1182 -->
<!-- npu="A3" id1183 -->
- Atlas A3 products: Supported
<!-- end id1183 -->
<!-- npu="910b" id1184 -->
- Atlas A2 products: Supported
<!-- end id1184 -->
<!-- npu="310b" id1185 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1185 -->
<!-- npu="310p" id1186 -->
- Atlas inference products: Supported
<!-- end id1186 -->
<!-- npu="910" id1187 -->
- Atlas training products: Supported
<!-- end id1187 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopCompile_res.md#id1 -->

## Feature Description

Compiles a specified operator. Before compiling the operator, you can call the [aclSetCompileopt](aclSetCompileopt.md) API to set compilation options.

Each operator has a different input and output organization. When calling the API, organize the operator strictly according to its input and output parameters. When you call aclopCompile, the API locates the corresponding task based on the optype, input tensor descriptions, output tensor descriptions, attr, and other information, and then compiles the operator.

## Prototype

```c
aclError aclopCompile(const char *opType,
int numInputs,
const aclTensorDesc *const inputDesc[],
int numOutputs,
const aclTensorDesc *const outputDesc[],
const aclopAttr *attr,
aclopEngineType engineType,
aclopCompileType compileFlag,
const char *opPath)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type name. |
| numInputs | Input | Number of input tensors of the operator. |
| inputDesc | Input | Pointer array to the descriptions of the operator input tensors. See [aclTensorDesc](aclTensorDesc.md) for type definition.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the inputDesc array must be consistent with the value of numInputs. |
| numOutputs | Input | Number of output tensors of the operator. |
| outputDesc | Input | Pointer array to the descriptions of the operator output tensors. See [aclTensorDesc](aclTensorDesc.md) for type definition.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the outputDesc array must be consistent with the value of numOutputs. |
| attr | Input | Pointer to the operator attributes. See [aclopAttr](aclopAttr.md) for type definition.<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |
| engineType | Input | Operator execution engine. See [aclopEngineType](aclopEngineType.md) for type definition. |
| compileFlag | Input | Operator compilation flag. See [aclopCompileType](aclopCompileType.md) for type definition. |
| opPath | Input | Pointer to the path of the operator implementation file (*.py), excluding the file name. Reserved parameter. Currently only nullptr is supported. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

- When compiling an operator with dynamic shapes, if the specific shape values are not determined but the shape ranges are determined, set the element values representing dynamic dimensions in the dims array to -1 when calling the aclCreateTensorDesc API to create the [aclTensorDesc](aclTensorDesc.md) type, and then call the [aclSetTensorShapeRange](aclSetTensorShapeRange.md) API to set the value range of each dimension of the tensor. If neither the specific shape values nor the shape ranges are determined (this scenario is reserved), set the values in the dims array to -2 when calling the aclCreateTensorDesc API to create the [aclTensorDesc](aclTensorDesc.md) type, for example: int64\_t dims\[\] = \{-2\}.
- When compiling an operator with optional inputs, if an optional input is not used, call the aclCreateTensorDesc(ACL\_DT\_UNDEFINED, 0, nullptr, ACL\_FORMAT\_UNDEFINED) API to create data of the [aclTensorDesc](aclTensorDesc.md) type, set the data type to ACL\_DT\_UNDEFINED, set Format to ACL\_FORMAT\_UNDEFINED, and set the shape information to nullptr.
- When compiling an operator with constant inputs, call the [aclSetTensorConst](aclSetTensorConst.md) API first to set the constant inputs. The constant input data set before calling the aclopCompile API and the [aclopExecuteV2](aclopExecuteV2.md) API must remain consistent.
