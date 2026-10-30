# aclopUpdateParams

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:58:35.646Z pushedAt=2026-10-29T03:59:00.806Z nl=2 -->

## Product Support

<!-- npu="950" id260 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id260 -->
<!-- npu="A3" id261 -->
- Atlas A3 products: Supported
<!-- end id261 -->
<!-- npu="910b" id262 -->
- Atlas A2 products: Supported
<!-- end id262 -->
<!-- npu="310b" id263 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id263 -->
<!-- npu="310p" id264 -->
- Atlas inference products: Supported
<!-- end id264 -->
<!-- npu="910" id265 -->
- Atlas training products: Supported
<!-- end id265 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopUpdateParams_res.md#id1 -->

## Feature Description

In the dynamic shape scenario of operators, triggers the operator selector based on the specified operator.

## Prototype

```c
aclError aclopUpdateParams(const char *opType,
int numInputs,
const aclTensorDesc *const inputDesc[],
int numOutputs,
const aclTensorDesc *const outputDesc[],
const aclopAttr *attr)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type name. |
| numInputs | Input | Number of input tensors of the operator. |
| inputDesc | Input | Pointer array to the descriptions of the operator input tensors. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the inputDesc array must be consistent with the value of numInputs. |
| numOutputs | Input | Number of output tensors of the operator. |
| outputDesc | Input | Pointer array to the descriptions of the operator output tensors. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the outputDesc array must be consistent with the value of numOutputs. |
| attr | Input | Pointer to the operator attributes. See [aclopAttr](aclopAttr.md) for the type definition.<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
