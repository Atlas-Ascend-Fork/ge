# aclTransTensorDescFormat

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:06:18.834Z pushedAt=2026-10-29T04:01:32.697Z nl=2 -->

**Notice: This API is reserved and not supported yet.**

## Product Support

<!-- npu="950" id92 -->
- Ascend 950PR&950DT products: Supported
<!-- end id92 -->
<!-- npu="A3" id93 -->
- Atlas A3 products: Supported
<!-- end id93 -->
<!-- npu="910b" id94 -->
- Atlas A2 products: Supported
<!-- end id94 -->
<!-- npu="310b" id95 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id95 -->
<!-- npu="310p" id96 -->
- Atlas inference products: Supported
<!-- end id96 -->
<!-- npu="910" id97 -->
- Atlas training products: Supported
<!-- end id97 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclTransTensorDescFormat_res.md#id1 -->

## Feature Description

Converts the format in the source aclTensorDesc according to the specified dstFormat to generate a new target aclTensorDesc. The format in the source aclTensorDesc remains unchanged. This is a synchronous API.

## Prototype

```c
aclError aclTransTensorDescFormat(const aclTensorDesc *srcDesc, aclFormat dstFormat, aclTensorDesc **dstDesc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| srcDesc | Input | Pointer to the source aclTensorDesc data.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| dstFormat | Input | Target format to set. See aclFormat for the type definition. |
| dstDesc | Output | Pointer to the pointer to the target aclTensorDesc data.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
