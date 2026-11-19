# aclmdlDestroyExecConfigHandle

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:11:36.554Z pushedAt=2026-11-18T03:28:46.450Z nl=2 -->

## Product Support

<!-- npu="950" id601 -->
- Ascend 950PR&950DT products: Supported
<!-- end id601 -->
<!-- npu="A3" id602 -->
- Atlas A3 products: Supported
<!-- end id602 -->
<!-- npu="910b" id603 -->
- Atlas A2 products: Supported
<!-- end id603 -->
<!-- npu="310b" id604 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id604 -->
<!-- npu="310p" id605 -->
- Atlas inference products: Supported
<!-- end id605 -->
<!-- npu="910" id606 -->
- Atlas training products: Supported
<!-- end id606 -->
<!-- npu="IPV350" id607 -->
- IPV350: support
<!-- end id607 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlDestroyExecConfigHandle_res.md#id1 -->

## Function Description

Destroys the aclmdlExecConfigHandle data created by the [aclmdlCreateExecConfigHandle](aclmdlCreateExecConfigHandle.md) API.

## Prototype

```c
aclError aclmdlDestroyExecConfigHandle(const aclmdlExecConfigHandle *handle)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| handle | Input | Pointer to the aclmdlExecConfigHandle to be destroyed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
