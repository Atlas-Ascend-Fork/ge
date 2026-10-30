# aclopCreateHandle

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:46:02.984Z pushedAt=2026-10-29T03:43:03.203Z nl=2 -->

## Product Support

<!-- npu="950" id986 -->
- Ascend 950PR&950DT products: Supported
<!-- end id986 -->
<!-- npu="A3" id987 -->
- Atlas A3 products: Supported
<!-- end id987 -->
<!-- npu="910b" id988 -->
- Atlas A2 products: Supported
<!-- end id988 -->
<!-- npu="310b" id989 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id989 -->
<!-- npu="310p" id990 -->
- Atlas inference products: Supported
<!-- end id990 -->
<!-- npu="910" id991 -->
- Atlas training products: Supported
<!-- end id991 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopCreateHandle_res.md#id1 -->

## Feature Description

Creates a handle for executing an operator.

To destroy the handle, see [aclopDestroyHandle](aclopDestroyHandle.md).

## Prototype

```c
aclError aclopCreateHandle(const char *opType,
int numInputs,
const aclTensorDesc *const inputDesc[],
int numOutputs,
const aclTensorDesc *const outputDesc[],
const aclopAttr *opAttr,
aclopHandle **handle)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type name. |
| numInputs | Input | Number of input tensors of the operator. |
| inputDesc | Input | Pointer array of input tensor descriptions of the operator. |
| numOutputs | Input | Number of output tensors of the operator. |
| outputDesc | Input | Pointer array of output tensor descriptions of the operator. See [aclTensorDesc](aclTensorDesc.md) for type definition. |
| opAttr | Input | Pointer to the operator attributes. See [aclopAttr](aclopAttr.md) for type definition. |
| handle | Output | Pointer to the aclopHandle data pointer. See [aclopHandle](aclopHandle.md) for type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
## Constraints

For operators with constant inputs, if [aclSetTensorConst](aclSetTensorConst.md) is not called to set the constant inputs, call [aclSetTensorPlaceMent](aclSetTensorPlaceMent.md) to set the placement attribute of TensorDesc and set memType to Host memory.
<!-- end id1 -->
