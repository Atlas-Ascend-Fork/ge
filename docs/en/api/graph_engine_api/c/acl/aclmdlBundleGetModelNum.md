# aclmdlBundleGetModelNum

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:54:23.184Z pushedAt=2026-11-18T03:17:24.416Z nl=2 -->

## Product Support

<!-- npu="950" id811 -->
- Ascend 950PR&950DT products: Supported
<!-- end id811 -->
<!-- npu="A3" id812 -->
- Atlas A3 products: Supported
<!-- end id812 -->
<!-- npu="910b" id813 -->
- Atlas A2 products: Supported
<!-- end id813 -->
<!-- npu="310b" id814 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id814 -->
<!-- npu="310p" id815 -->
- Atlas inference products: Supported
<!-- end id815 -->
<!-- npu="910" id816 -->
- Atlas training products: Supported
<!-- end id816 -->
<!-- npu="IPV350" id817 -->
- IPV350: not supported
<!-- end id817 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleGetModelNum_res.md#id1 -->

## Function Description

Obtains the actual number of executable graphs based on bundleId.

## Prototype

```c
aclError aclmdlBundleGetModelNum(uint32_t bundleId, size_t *modelNum)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| bundleId | Input | bundleId returned after the model is successfully loaded by the [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md) API or the [aclmdlBundleLoadFromMem](aclmdlBundleLoadFromMem.md) API. |
| modelNum | Output | Actual number of executable graphs. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
