# Performance Tuning

<!-- md-trans-meta sourceCommit=03b8f43b8d6d016735ca55a32b59cd73bc779bda translatedAt=2026-11-17T02:47:23.820Z pushedAt=2026-11-18T02:41:08.891Z nl=2 -->

## ENABLE\_SMALL\_CHANNEL

Whether to enable Small Channel optimization. When enabled, performance gains are achieved for convolution layers with Channel <= 4. It is recommended that users enable this switch in inference scenarios. The corresponding **options** parameter for this parameter is `ge.enableSmallChannel`.

**Parameter values**

- **0**: (Default) Disabled.
- **1**: Enabled.

**Configuration example:**

```c++
{ge::ir_option::ENABLE_SMALL_CHANNEL, "1"}
```

**Applicable products:**

Supported on all chips.

<!-- npu="950,A3,910b,910,310p,310b,IPV350" id8 -->
## TILING\_SCHEDULE\_OPTIMIZE

Switch for Tiling offloading scheduling optimization. The corresponding **options** parameter for this parameter is `ge.tiling_schedule_optimize`.

Because the internal storage of the AI Core in the NPU cannot fully hold all input and output data of an operator, a portion of the input data must be moved in for computation and then moved out each time, followed by moving in the next portion of input data for computation. This process is called Tiling. The computation program that determines the parameters of the data splitting algorithm (such as the block size moved each time and the total number of loops) based on information such as the operator's shape is called the Tiling implementation. Since all computations performed in the Tiling implementation are scalar computations, which the AI Core is not good at, they are generally executed on the Host-side CPU. However, the Tiling implementation is offloaded to the device for execution when the following conditions are met:

1. The model has a static shape.
2. The operators in the model support Tiling offloading, such as fused operators like **FusedInferAttentionScore** and **IncreFlashAttention**.
3. The values of operators that support Tiling offloading have dependencies, and the value of the preceding operator must have a Device-side execution result. If the dependent value is a Const, Tiling does not need to be offloaded for execution, and Tiling is completed at compile time.

**Parameter values:**

- **0**: Disables Tiling offloading. The default value is **0**.
- **1**: Enables Tiling offloading.

**Configuration example:**

```c++
{ge::ir_option::TILING_SCHEDULE_OPTIMIZE, "1"}
```

**Usage constraints:**
When [OFFLINE_MODE](../aclgrphBuildModel_config_params/basic_functions.md#offline_mode) is set to 7, this parameter is not supported.

**Applicable products:**

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Not supported
<!-- end id6 -->
<!-- npu="IPV350" id7 -->
- IPV350: Not supported
<!-- end id7 -->

<!-- end id8 -->
