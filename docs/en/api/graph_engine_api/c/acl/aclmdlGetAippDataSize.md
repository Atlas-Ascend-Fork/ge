# aclmdlGetAippDataSize

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:08:28.126Z pushedAt=2026-10-29T03:06:42.727Z nl=2 -->

## Product Support

<!-- npu="950" id972 -->
- Ascend 950PR&950DT products: Supported
<!-- end id972 -->
<!-- npu="A3" id973 -->
- Atlas A3 products: Supported
<!-- end id973 -->
<!-- npu="910b" id974 -->
- Atlas A2 products: Supported
<!-- end id974 -->
<!-- npu="310b" id975 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id975 -->
<!-- npu="310p" id976 -->
- Atlas inference products: Supported
<!-- end id976 -->
<!-- npu="910" id977 -->
- Atlas training products: Supported
<!-- end id977 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetAippDataSize_res.md#id1 -->

## Feature Description

Obtains the memory size required for dynamic AIPP input data based on batchSize.

## Prototype

```c
aclError aclmdlGetAippDataSize(uint64_t batchSize, size_t *size)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| batchSize | Input | Batch size of the original model. |
| size | Output | Memory size, in bytes. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
