# aclDestroyGraphDumpOpt

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:36:04.958Z pushedAt=2026-10-29T02:32:59.039Z nl=2 -->

## Product Support

<!-- npu="950" id71 -->
- Ascend 950PR&950DT products: Supported
<!-- end id71 -->
<!-- npu="A3" id72 -->
- Atlas A3 products: Supported
<!-- end id72 -->
<!-- npu="910b" id73 -->
- Atlas A2 products: Supported
<!-- end id73 -->
<!-- npu="310b" id74 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id74 -->
<!-- npu="310p" id75 -->
- Atlas inference products: Supported
<!-- end id75 -->
<!-- npu="910" id76 -->
- Atlas training products: Supported
<!-- end id76 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclDestroyGraphDumpOpt_res.md#id1 -->

## Feature Description

Destroys the aclGraphDumpOption data created by the [aclCreateGraphDumpOpt](aclCreateGraphDumpOpt.md) API.

## Prototype

```c
aclError aclDestroyGraphDumpOpt(const aclGraphDumpOption *graphDumpOpt)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graphDumpOpt | Input | Pointer to the aclGraphDumpOption data to be destroyed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
