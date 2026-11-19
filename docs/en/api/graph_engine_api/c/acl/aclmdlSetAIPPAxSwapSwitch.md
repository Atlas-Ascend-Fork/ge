# aclmdlSetAIPPAxSwapSwitch

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:42:54.859Z pushedAt=2026-11-18T03:36:58.950Z nl=2 -->

## Product Support

<!-- npu="950" id909 -->
- Ascend 950PR&950DT products: Supported
<!-- end id909 -->
<!-- npu="A3" id910 -->
- Atlas A3 products: Supported
<!-- end id910 -->
<!-- npu="910b" id911 -->
- Atlas A2 products: Supported
<!-- end id911 -->
<!-- npu="310b" id912 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id912 -->
<!-- npu="310p" id913 -->
- Atlas inference products: Supported
<!-- end id913 -->
<!-- npu="910" id914 -->
- Atlas training products: Supported
<!-- end id914 -->
<!-- npu="IPV350" id915 -->
- IPV350: not supported
<!-- end id915 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPAxSwapSwitch_res.md#id1 -->

## Function Description

In dynamic AIPP scenarios, set the swap switch for RGBA-\>ARGB or YUVA-\>AYUV.

## Prototype

```c
aclError aclmdlSetAIPPAxSwapSwitch(aclmdlAIPP *aippParmsSet, int8_t axSwapSwitch)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| axSwapSwitch | Input | Whether to swap RGBA->ARGB or YUVA->AYUV. Value range:<br><br>  - 0: No swap<br>  - 1: Swap |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
