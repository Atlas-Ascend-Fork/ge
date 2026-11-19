# Dynamic Shape

<!-- md-trans-meta sourceCommit=03b8f43b8d6d016735ca55a32b59cd73bc779bda translatedAt=2026-11-17T02:45:13.400Z pushedAt=2026-11-18T02:41:08.884Z nl=2 -->

## AC\_PARALLEL\_ENABLE

<!-- npu="950,A3,910b,910,310p" id1 -->
In a dynamic shape graph, whether AI CPU operators and AI Core operators are allowed to run in parallel. The **options** parameter that actually corresponds to this parameter is `ac_parallel_enable`.

In a dynamic shape graph, when the switch is enabled, the system automatically identifies AI CPU operators that can run concurrently with the AI Core, and dispatches operators of different engines to different streams to implement parallelism among multiple engines, thereby improving resource utilization efficiency and dynamic shape execution performance.

**Parameter values:**

- **1**: Allows AI CPU and AI Core operators to run in parallel.
- **0** (default): AI CPU operators are not dispatched to a separate stream.

**Configuration example:**

```c++
{ge::ir_option::AC_PARALLEL_ENABLE, "1"}
```
<!-- end id1 -->

<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/dynamic_shape_res.md#id1 -->

**Usage constraints:**
When the value of [OFFLINE_MODE](../aclgrphBuildModel_config_params/basic_functions.md#offline_mode) is 7, this parameter is not supported.

**Applicable products:**

<!-- npu="950" id2 -->
- Ascend 950PR&950DT series products: Supported
<!-- end id2 -->
<!-- npu="A3" id3 -->
- Atlas A3 series products: Supported
<!-- end id3 -->
<!-- npu="910b" id4 -->
- Atlas A2 products: Supported
<!-- end id4 -->
<!-- npu="310b" id5 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id5 -->
<!-- npu="310p" id6 -->
- Atlas inference products: Supported
<!-- end id6 -->
<!-- npu="910" id7 -->
- Atlas training products: Supported
<!-- end id7 -->
<!-- npu="IPV350" id8 -->
- IPV350: Not supported
<!-- end id8 -->
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/dynamic_shape_res.md#id2 -->
