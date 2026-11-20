# aclmdlGetAippDataSize

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:14:46.428Z pushedAt=2026-11-18T03:29:33.680Z nl=2 -->

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
<!-- npu="IPV350" id978 -->
- IPV350: not supported
<!-- end id978 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetAippDataSize_res.md#id1 -->

## Function Description

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
