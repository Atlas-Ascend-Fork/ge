# --shape\_generalized\_build\_mode

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:40:59.730Z pushedAt=2026-10-27T03:49:15.484Z -->

## Feature Overview

**This parameter will be deprecated in later versions. Do not use it.**

Compilation mode of Shape during graph compilation.

## Related Parameters

This parameter cannot be used together with [--input\_shape\_range](--input_shape_range.md), [--dynamic\_batch\_size](--dynamic_batch_size.md), [--dynamic\_image\_size](--dynamic_image_size.md), or [--dynamic\_dims](--dynamic_dims.md).

## Parameter Values

**Parameter value:**

- shape\_generalized: fuzzy compilation. During compilation, the system generalizes the variable dimensions before compiling. If the operator shape is fixed, the variable dimensions are changed to -1 (the number of dimensions remains unchanged; for example, if the original shape has four dimensions, it still has four dimensions after fuzzy compilation) before compilation.

    This parameter applies to the scenario where you want to compile once and perform inference multiple times. In this case, you can use the fuzzy compilation feature.

- shape\_precise: (default) precise compilation. It compiles directly according to the dimension information specified by the user, without any escaping during compilation.

**Parameter value constraints:**
If an operator does not support dynamic shapes and supports only static shapes (no variable dimensions), the operator is compiled according to the static shape, and no generalization is performed for fuzzy compilation.

[Figure 1](#fig1) shows the two compilation modes.

**Figure 1**  Compilation modes<a id="fig1"></a>
![Illustration](../figures/compilation_mode.png "Compilation modes")

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --shape_generalized_build_mode=shape_generalized ...
```

## Usage Constraints

If fuzzy compilation is set through this parameter during model conversion, when using the application project for model inference, you need to add the [aclmdlSetDatasetTensorDesc](../../../api/graph_engine_api/c/acl/aclmdlSetDatasetTensorDesc.md) API before the [aclmdlExecute](../../../api/graph_engine_api/c/acl/aclmdlExecute.md) API to set the actual shape values.
