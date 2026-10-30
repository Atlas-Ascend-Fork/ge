# aclopCreateHandleForCast

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:46:20.946Z pushedAt=2026-10-29T03:43:09.127Z nl=2 -->

## Product Support

<!-- npu="950" id574 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id574 -->
<!-- npu="A3" id575 -->
- Atlas A3 products: Supported
<!-- end id575 -->
<!-- npu="910b" id576 -->
- Atlas A2 products: Supported
<!-- end id576 -->
<!-- npu="310b" id577 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id577 -->
<!-- npu="310p" id578 -->
- Atlas inference products: Supported
<!-- end id578 -->
<!-- npu="910" id579 -->
- Atlas training products: Supported
<!-- end id579 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopCreateHandleForCast_res.md#id1 -->

## Feature Description

Creates a handle for data type conversion. After the handle is created, call the [aclopExecWithHandle](aclopExecWithHandle.md) API to execute the operator.

## Prototype

```c
aclError aclopCreateHandleForCast(aclTensorDesc *srcDesc,
aclTensorDesc *dstDesc,
uint8_t truncate,
aclopHandle **handle)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| srcDesc | Input | Pointer to the input tensor descriptor. See [aclTensorDesc](aclTensorDesc.md) for the type definition. |
| dstDesc | Input | Pointer to the output tensor descriptor. See [aclTensorDesc](aclTensorDesc.md) for the type definition. |
| truncate | Input | Reserved. |
| handle | Output | Pointer to a pointer to the operator execution handle. See [aclopHandle](aclopHandle.md) for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
