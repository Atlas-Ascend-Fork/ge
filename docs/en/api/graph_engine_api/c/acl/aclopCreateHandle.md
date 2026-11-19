# aclopCreateHandle

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:59:53.214Z pushedAt=2026-11-18T03:41:09.460Z nl=2 -->

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
<!-- npu="IPV350" id992 -->
- IPV350: not supported
<!-- end id992 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopCreateHandle_res.md#id1 -->

## Function Description

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

For scenarios where an operator has constant inputs, if the [aclSetTensorConst](aclSetTensorConst.md) API is not called to set the constant inputs, call the [aclSetTensorPlaceMent](aclSetTensorPlaceMent.md) API to set the placement attribute of the TensorDesc and set memType to Host memory.
<!-- end id1 -->
