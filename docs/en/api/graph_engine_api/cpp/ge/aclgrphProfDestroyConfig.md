# aclgrphProfDestroyConfig

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:09:56.841Z pushedAt=2026-10-28T07:40:22.124Z nl=2 -->

## Applicable Products

<!-- npu="950" id1 -->
- Ascend 950PR & 950DT series products: Supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 series products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 series products: Supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Supported
<!-- end id6 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphProfDestroyConfig_res.md#id1 -->

## Header File/Library File

- Header file: \#include <ge/ge\_prof.h\>
- Library file: libmsprofiler.so

## Description

Destroys the profiling configuration information.

## Prototype

```c++
Status aclgrphProfDestroyConfig(aclgrphProfConfig *profiler_config)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| profiler_config | Input | Profiling configuration information structure pointer. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | SUCCESS: Success.<br>Other: Failure. |

## Constraints

- Use this API together with [aclgrphProfCreateConfig](aclgrphProfCreateConfig.md). Call **aclgrphProfCreateConfig** first and then call **aclgrphProfDestroyConfig**.
- The profiling configuration information to be destroyed must be created by **aclgrphProfCreateConfig**.
- If destroying the profiling configuration information fails, check whether the configuration information parameters are valid.
