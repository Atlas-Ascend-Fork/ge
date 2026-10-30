# aclopSetCompileFlag

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:55:58.061Z pushedAt=2026-10-29T03:57:55.708Z nl=2 -->

## Product Support

<!-- npu="950" id958 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id958 -->
<!-- npu="A3" id959 -->
- Atlas A3 products: Supported
<!-- end id959 -->
<!-- npu="910b" id960 -->
- Atlas A2 products: Supported
<!-- end id960 -->
<!-- npu="310b" id961 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id961 -->
<!-- npu="310p" id962 -->
- Atlas inference products: Supported
<!-- end id962 -->
<!-- npu="910" id963 -->
- Atlas training products: Supported
<!-- end id963 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetCompileFlag_deprecated_res.md#id1 -->

## Feature Description

Call this API to set the operator compilation flag before compiling an operator. The setting is shared globally at the process level.

## Prototype

```c
aclError aclopSetCompileFlag(aclOpCompileFlag flag)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| flag | Input | Used to set the operator compilation flag. See [aclOpCompileFlag](aclOpCompileFlag.md) for the type definition.<br>If flag is not set, exact compilation is used by default. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
