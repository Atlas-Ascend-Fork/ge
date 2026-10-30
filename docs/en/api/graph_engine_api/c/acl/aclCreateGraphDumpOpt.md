# aclCreateGraphDumpOpt

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:35:18.869Z pushedAt=2026-10-28T13:15:24.626Z nl=2 -->

## Product Support

<!-- npu="950" id323 -->
- Ascend 950PR&950DT products: Supported
<!-- end id323 -->
<!-- npu="A3" id324 -->
- Atlas A3 products: Supported
<!-- end id324 -->
<!-- npu="910b" id325 -->
- Atlas A2 products: Supported
<!-- end id325 -->
<!-- npu="310b" id326 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id326 -->
<!-- npu="310p" id327 -->
- Atlas inference products: Supported
<!-- end id327 -->
<!-- npu="910" id328 -->
- Atlas training products: Supported
<!-- end id328 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclCreateGraphDumpOpt_res.md#id1 -->

## Feature Description

Creates data of the aclGraphDumpOption type, which represents the Dump configuration information of a single-operator graph and is used to set options for graph Dump. Currently no options are available; this data type is reserved for future extension.

## Prototype

```c
aclGraphDumpOption *aclCreateGraphDumpOpt()
```

## Parameters

None

## Return Value

- Returns a pointer of the aclGraphDumpOption type on success.
- Returns nullptr on failure.
