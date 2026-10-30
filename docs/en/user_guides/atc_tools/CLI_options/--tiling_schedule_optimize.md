# --tiling\_schedule\_optimize

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:44:48.177Z pushedAt=2026-10-27T13:26:38.678Z -->

## Product Support

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

## Feature Description

Switch for Tiling offload scheduling optimization.

Tiling offload refers to performing Tiling computation on the CPU on the Device side. Because the internal storage of the AI Core in an NPU cannot fully accommodate all input and output data of an operator, a portion of the input data must be moved in for computation and then moved out each time, before the next portion of input data is moved in for computation. This process is called Tiling. The computation program that determines the parameters related to the data splitting algorithm (such as the block size moved each time and the total number of loops) based on information such as the operator shape is called the Tiling implementation. Because the Tiling implementation performs only scalar computation, which the AI Core is not good at, it is generally executed on the CPU on the Host side. However, the Tiling implementation is offloaded to the Device side for execution when the following conditions are met:

1. The model has a static shape.
2. The operators in the model support Tiling offload, such as fused operators like FusedInferAttentionScore and IncreFlashAttention.
3. The values of the operators that support Tiling offload have dependencies, and the value of the preceding operator must have an execution result on the Device. If the dependent value is a Const, Tiling offload is not required, and Tiling is completed at compile time.

## Parameters

- 0: (default) Disables Tiling sinking.
- 1: Enables Tiling sinking.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --tiling_schedule_optimize=1 ...
```
