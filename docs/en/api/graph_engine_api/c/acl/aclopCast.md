# aclopCast

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:42:56.479Z pushedAt=2026-10-29T03:42:05.868Z nl=2 -->

## Product Support

<!-- npu="950" id650 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id650 -->
<!-- npu="A3" id651 -->
- Atlas A3 products: Supported
<!-- end id651 -->
<!-- npu="910b" id652 -->
- Atlas A2 products: Supported
<!-- end id652 -->
<!-- npu="310b" id653 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id653 -->
<!-- npu="310p" id654 -->
- Atlas inference products: Supported
<!-- end id654 -->
<!-- npu="910" id655 -->
- Atlas training products: Supported
<!-- end id655 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopCast_res.md#id1 -->

## Feature Description

Converts the data type of the input data. This is an asynchronous API.

## Prototype

```c
aclError aclopCast(const aclTensorDesc *srcDesc,
const aclDataBuffer *srcBuffer,
const aclTensorDesc *dstDesc,
aclDataBuffer *dstBuffer,
uint8_t truncate,
aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| srcDesc | Input | Pointer to the input tensor description. See [aclTensorDesc](aclTensorDesc.md) for the type definition. |
| srcBuffer | Input | Pointer to the input tensor. See aclDataBuffer for the type definition. |
| dstDesc | Input | Pointer to the output tensor description. See [aclTensorDesc](aclTensorDesc.md) for the type definition. |
| dstBuffer | Output | Pointer to the output tensor. See aclDataBuffer for the type definition. |
| truncate | Input | Reserved. |
| stream | Input | Stream on which the operator runs. See aclrtStream for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
