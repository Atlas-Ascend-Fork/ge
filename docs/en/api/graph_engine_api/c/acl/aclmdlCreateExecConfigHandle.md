# aclmdlCreateExecConfigHandle

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:05:53.949Z pushedAt=2026-11-18T03:19:46.675Z nl=2 -->

## Product Support

<!-- npu="950" id365 -->
- Ascend 950PR&950DT products: Supported
<!-- end id365 -->
<!-- npu="A3" id366 -->
- Atlas A3 products: Supported
<!-- end id366 -->
<!-- npu="910b" id367 -->
- Atlas A2 products: Supported
<!-- end id367 -->
<!-- npu="310b" id368 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id368 -->
<!-- npu="310p" id369 -->
- Atlas inference products: Supported
<!-- end id369 -->
<!-- npu="910" id370 -->
- Atlas training products: Supported
<!-- end id370 -->
<!-- npu="IPV350" id371 -->
- IPV350: support
<!-- end id371 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlCreateExecConfigHandle_res.md#id1 -->

## Function Description

Creates data of the aclmdlExecConfigHandle type, which represents a configuration object for model execution.

To destroy data of the aclmdlExecConfigHandle type, see [aclmdlDestroyExecConfigHandle](aclmdlDestroyExecConfigHandle.md).

## Prototype

```c
aclmdlExecConfigHandle *aclmdlCreateExecConfigHandle()
```

## Parameters

None

## Return Value

- Returns a pointer of the aclmdlExecConfigHandle type, indicating success.
- Returns NULL, indicating failure.
