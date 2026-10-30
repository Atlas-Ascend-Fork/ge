# --sparsity

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:42:40.869Z pushedAt=2026-10-27T13:08:33.153Z -->

## Product Support

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: not supported
<!-- end id1 -->

<!-- npu="A3" id2 -->
- Atlas A3 products: supported
<!-- end id2 -->

<!-- npu="910b" id3 -->
- Atlas A2 products: supported
<!-- end id3 -->

<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: supported
<!-- end id4 -->

<!-- npu="310p" id5 -->
- Atlas inference products: not supported
<!-- end id5 -->

<!-- npu="910" id6 -->
- Atlas training products: not supported
<!-- end id6 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--sparsity_res.md#id1 -->

## Feature Overview

Enables the global sparse feature.

A model output by AMCT (Ascend Model Compression Toolkit) after 4-to-2 structured sparsification may contain scenarios where at least 2 of every 4 consecutive Cin dimension elements in weight are 0. During model conversion, enabling the global sparse switch filters the elements in such scenarios down to 2, thereby reducing the computation workload of subsequent inference and improving inference performance. The principle is as follows:

For every 4 consecutive elements in weight, two groups of 2-bit indexes are generated. The index value range is \{0, 1, 2\}, where the first index indicates the position of the first non-zero element among the first three elements, and the second index indicates the position of the last non-zero element among the last three elements. The final filtered index values are shown in the following table:

**Table 1** Filtering rules

| Scenario | ele0 | ele1 | ele2 | ele3 | index[0] | index[1] |
| --- | --- | --- | --- | --- | --- | --- |
| 2 non-zero elements | 0 | 0 | X | Y | 2'b10 | 2'b10 |
| 2 non-zero elements | 0 | X | 0 | Y | 2'b01 | 2'b10 |
| 1 non-zero element | 0 | 0 | 0 | X | 2'b00 | 2'b10 |
| 1 non-zero element | 0 | 0 | X | 0 | 2'b10 | 2'b01 |
| All-zero elements | 0 | 0 | 0 | 0 | 2'b00 | 2'b00 |

Because every 4 int8 elements of the sparse matrix generate 2 2-bit indexes and 2 int8 dense weight elements, and one index is 2 bits, 4 indexes form one int8 element. Therefore, the size of the final output index matrix is 1/4 of that of the dense weight. Specifically:

The index matrix records the indexes of the 2 elements selected from every 4 elements of the sparse weight. It is read by the hardware during inference computation and serves as the basis for filtering elements.

The interaction process of each component is shown in the following figure:

**Figure 1** Interaction process
![Diagram](../figures/interaction_flow.png "Interaction process")

1. The AMCT tool provides a structured sparse API for users, supporting the conversion of a dense model into a structured sparse model through retraining. It outputs a framework-based sparse model. The model structure is identical to that of the dense model, but structured sparsity has been implemented in the parameters, with 2 of every 4 consecutive Cin dimension elements being zero.
2. Based on sparsity=1, GE traverses the weight of all operator types that support structured sparsity (currently Conv2D/MatMulV2/FullyConnection) to check whether the current parameter distribution can adopt the 2-of-4 structured sparse feature. It inserts CompressOp (reusing the CompressOp operator prototype for weight compression and adding an Alg attribute to distinguish weight compression from structured sparsity) to complete the weight rearrangement and index generation for structured sparsity.
3. If the 4-out-of-2 structured sparse execution condition is met, the operators in the original framework that support the sparse feature, such as Conv2D, are replaced with Conv2DCompress. The prototype of this type of operator adds an Alg attribute to distinguish whether the feature used is weight compression or the 4-out-of-2 structured sparse feature.

## Related Parameters

None.

## Parameter Values

- 1: enables 2-out-of-4 structured sparse.
- 0: (default) does not enable the sparse feature.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --sparsity=1 ...
```

## Usage Constraints

When using this parameter, ensure that the model is a sparse model. It is recommended that you obtain the model using the combined compression function of AMCT (TensorFlow) or AMCT (PyTorch), and the combined compression must be in the 4-out-of-2 structured sparse + quantization-aware training mode.
