# aclgrphProfCreateConfig

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:09:09.951Z pushedAt=2026-10-28T07:40:22.121Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphProfCreateConfig_res.md#id1 -->

## Header File/Library File

- Header file: \#include <ge/ge\_prof.h\>
- Library file: libmsprofiler.so

## Description

Creates profiling configuration information.

## Prototype

```c++
aclgrphProfConfig *aclgrphProfCreateConfig(uint32_t *deviceid_list, uint32_t device_nums, ProfilingAicoreMetrics aicore_metrics, ProfAicoreEvents *aicore_events, uint64_t data_type_config)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| deviceid_list | Input | List of device IDs whose data is to be collected. |
| device_nums | Input | Number of devices. Ensure that it is consistent with the number of devices in **deviceid_list**. |
| aicore_metrics | Input | AI Core metrics. For details about the enumeration values, see [ProfilingAicoreMetrics](ProfilingAicoreMetrics.md). |
| aicore_events | Input | AI Core events. Reserved item. |
| data_type_config | Input | Scope of profiling data content to be collected. For details, see [ProfDataTypeConfig](ProfDataTypeConfig.md).<br>The value can be combined by bitwise OR to specify multiple types of data to be output. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| aclgrphProfConfig | aclgrphProfConfig * | Pointer to the profiling configuration information struct. |

## Constraints

- Data of the **aclgrphProfConfig** type can be created once and used in multiple places. The user must ensure data consistency and accuracy.
- This API is used in pair with [aclgrphProfDestroyConfig](aclgrphProfDestroyConfig.md). Call **aclgrphProfCreateConfig** first and then **aclgrphProfDestroyConfig**.
- The user must ensure that **aclgrphProfDestroyConfig** is called at the end of the program to destroy all created profiling configuration information; otherwise, memory leaks may occur.
- To specify different profiling configuration information on different devices, the user can create different data of the **aclgrphProfConfig** type and call [aclgrphProfStart](aclgrphProfStart.md) in sequence to deliver different configuration information to different devices. Note that device information must not be duplicated.
