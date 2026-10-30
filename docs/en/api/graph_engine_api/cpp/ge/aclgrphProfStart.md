# aclgrphProfStart

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:11:13.418Z pushedAt=2026-10-28T07:40:22.128Z nl=2 -->

## Applicable Products

<!-- npu="950" id1 -->
- Ascend 950PR&950DT: Supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2: Supported
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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphProfStart_res.md#id1 -->

## Header File/Library File

- Header file: \#include <ge/ge\_prof.h\>
- Library file: libmsprofiler.so

## Description

Sends a profiling request to start collecting the corresponding data.

## Prototype

```c++
Status aclgrphProfStart(aclgrphProfConfig *profiler_config)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| profiler_config | Input | Pointer to the profiling configuration information struct. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | SUCCESS: Success<br>Others: Failure |

## Constraints

- This API must be called before [RunGraph](./Session/RunGraph.md). If it is called during model execution, the profile data collected is the data after **aclgrphProfStart** is called, which may be incomplete.
- This API is used together with **aclgrphProfStop**. Call **aclgrphProfStart** first and then **aclgrphProfStop**.
- **aclgrphProfInit** \> **aclgrphProfStart** \> **aclgrphProfStop** \> **aclgrphProfFinalize** is a complete API call flow. If a user switches models or graphs within a single process to perform multiple rounds of execution, the complete flow described above must also be followed. Out-of-order calls or multiple parallel calls of an API are not supported.
