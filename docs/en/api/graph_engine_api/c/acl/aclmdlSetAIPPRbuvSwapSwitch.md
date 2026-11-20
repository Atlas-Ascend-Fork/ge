# aclmdlSetAIPPRbuvSwapSwitch

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:47:57.444Z pushedAt=2026-11-18T03:37:57.365Z nl=2 -->

## Product Support

<!-- npu="950" id267 -->
- Ascend 950PR&950DT products: Supported
<!-- end id267 -->
<!-- npu="A3" id268 -->
- Atlas A3 products: Supported
<!-- end id268 -->
<!-- npu="910b" id269 -->
- Atlas A2 products: Supported
<!-- end id269 -->
<!-- npu="310b" id270 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id270 -->
<!-- npu="310p" id271 -->
- Atlas inference products: Supported
<!-- end id271 -->
<!-- npu="910" id272 -->
- Atlas training products: Supported
<!-- end id272 -->
<!-- npu="IPV350" id273 -->
- IPV350: not supported
<!-- end id273 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlSetAIPPRbuvSwapSwitch_res.md#id1 -->

## Function Description

In dynamic AIPP scenarios, sets whether to swap the R and B channels, or whether to swap the U and V channels.

## Prototype

```c
aclError aclmdlSetAIPPRbuvSwapSwitch(aclmdlAIPP *aippParmsSet, int8_t rbuvSwapSwitch)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| aippParmsSet | Output | Pointer to the dynamic AIPP parameter object.<br>Call [aclmdlCreateAIPP](aclmdlCreateAIPP.md) in advance to create data of the aclmdlAIPP type. |
| rbuvSwapSwitch | Input | Switch that controls whether to swap the R and B channels, or the U and V channels. Value range:<br><br>  - 0: Do not swap<br>  - 1: Swap |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
