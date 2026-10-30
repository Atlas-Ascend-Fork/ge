# Dynamic Shape

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:31:27.724Z pushedAt=2026-10-28T07:40:22.188Z nl=2 -->

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

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/dynamic_shape_res.md#id1 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/dynamic_shape_res.md#id2 -->
