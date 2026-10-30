# aclmdlCreateExecConfigHandle

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:02:58.309Z pushedAt=2026-10-29T03:00:21.757Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlCreateExecConfigHandle_res.md#id1 -->

## Feature Description

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
