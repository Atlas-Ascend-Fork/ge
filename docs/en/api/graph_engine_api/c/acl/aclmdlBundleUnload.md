# aclmdlBundleUnload

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:01:28.833Z pushedAt=2026-11-18T03:18:45.601Z nl=2 -->

## Product Support

<!-- npu="950" id839 -->
- Ascend 950PR&950DT products: Supported
<!-- end id839 -->
<!-- npu="A3" id840 -->
- Atlas A3 products: Supported
<!-- end id840 -->
<!-- npu="910b" id841 -->
- Atlas A2 products: Supported
<!-- end id841 -->
<!-- npu="310b" id842 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id842 -->
<!-- npu="310p" id843 -->
- Atlas inference products: Supported
<!-- end id843 -->
<!-- npu="910" id844 -->
- Atlas training products: Supported
<!-- end id844 -->
<!-- npu="IPV350" id845 -->
- IPV350: not supported
<!-- end id845 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleUnload_res.md#id1 -->

## Function Description

After the system completes om/om2 model inference, you can call this API to unload the model loaded by [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md) or [aclmdlBundleLoadFromMem](aclmdlBundleLoadFromMem.md) and release resources.

**This API must be used together with other APIs** to dynamically update variables. For details, see the description of [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md).

## Prototype

```c
aclError aclmdlBundleUnload(uint32_t bundleId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| bundleId | Input | bundleId returned after the model is successfully loaded by the [aclmdlBundleLoadFromFile](aclmdlBundleLoadFromFile.md) API or the [aclmdlBundleLoadFromMem](aclmdlBundleLoadFromMem.md) API. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
