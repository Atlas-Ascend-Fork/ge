# Supported Layer Types and Constraints for Quantization

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:31:52.066Z pushedAt=2026-10-28T10:34:48.030Z nl=2 -->

This section lists the layers that can be quantized in different frameworks and the related constraints.

General description:

- If the input data type or weight data type of a network model is Float16 or a mixed precision type (Float32/Float16 coexisting), the quantization function of the following operators is disabled:
 AvgPool, Pooling, AvgPoolV2, MaxPool, MaxPoolV3, Add, Eltwise, and BatchMatMulV2 (both of the two inputs are variable tensors).
<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/references/quantization_layers_constraints_res.md#id1 -->

<!-- npu="A3,910b" id17 -->
- **The Caffe framework is no longer evolving in the following product forms, and its functions are not guaranteed to be available:**

    <!-- npu="A3" id3 -->
    Atlas A3 products
    <!-- end id3 -->

    <!-- npu="910b" id2 -->
    Atlas A2 products
    <!-- end id2 -->

<!-- end id17 -->

## Full Quantization

### Layers Supported by Uniform Quantization and Their Constraints (Caffe Framework)

| Supported Layer Types | Constraint | Layer Types Defined by Ascend IR |
| --- | --- | --- |
| InnerProduct: fully connected layer | transpose is false, axis is 1 | FullyConnection |
| Convolution: convolutional layer | filter dimension is 4 | Conv2D |
| Deconvolution: deconvolutional layer | dilation is 1, filter dimension is 4 | Deconvolution |
| Pooling | - mode is 1, full quantization (weight+tensor), global_pooling is false, shift N operation is not supported<br>  - mode is 0, only tensor quantization is performed | Pooling |
| Eltwise | only tensor quantization is performed and operation=1 | Eltwise |

### Layers Supported by Uniform Quantization and Their Constraints (TensorFlow Framework)

| Supported Layer Type | Constraint | Layer Type Defined by Ascend IR |
| --- | --- | --- |
| MatMul: fully connected layer | - transpose_a is False, transpose_b is False, adjoint_a is False, adjoint_b is False<br>  - The input source of weight does not contain dynamically changeable nodes such as placeholder | MatMulV2 |
| Conv2D: convolutional layer | The input source of weight does not contain dynamically changeable nodes such as placeholder | Conv2D |
| DepthwiseConv2dNative: depthwise convolutional layer | The input source of weight does not contain dynamically changeable nodes such as placeholder | DepthwiseConv2D |
| Conv2DBackpropInput | dilation is 1, and the input source of weight does not contain dynamically changeable nodes such as placeholder | Conv2DBackpropInput |
| BatchMatMulV2 | - adj_x=False<br>  - If the second input is a constant const, it must be 2-dimensional<br>  - When both inputs are variable tensors, only INT8 symmetric quantization is supported | BatchMatMulV2 |
| AvgPool | The shift N operation is not supported | AvgPool |
| Conv3D | dilation_d is 1 | Conv3D |
| MaxPool | Only tensor quantization is performed | MaxPool, MaxPoolV3 |
| Add | Only tensor quantization is performed, and only quantization of a single input is supported | Add |

<!-- npu="A3,910b,310b" id5 -->
For the BatchMatMulV2 quantization layer, in the scenario where both inputs are variable tensors, benefits can be obtained only on the following products. On other products, the accuracy decreases after quantization:

<!-- npu="A3" id6 -->
Atlas A3 products
<!-- end id6 -->

<!-- npu="910b" id7 -->
Atlas A2 products
<!-- end id7 -->

<!-- npu="310b" id8 -->
Atlas 200I/500 A2 inference product
<!-- end id8 -->
<!-- end id5 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/references/quantization_layers_constraints_res.md#id3 -->

### Layers and Constraints Supported by Uniform Quantization (ONNX Framework)

| Supported Layer Type | Constraint | Layer Type Defined by Ascend IR |
| --- | --- | --- |
| Conv: convolutional layer | - When the filter dimension is 5, dilation_d must be 1<br>  - The input source of weight does not contain dynamically changing nodes such as placeholder | Conv2D, Conv3D |
| Gemm: general matrix multiplication | - transpose_a=false<br>  - The input source of weight does not contain dynamically changing nodes such as placeholder | MatMulV2 |
| ConvTranspose: transposed convolution | - dilation is 1 and the filter dimension is 4<br>  - The input source of weight does not contain dynamically changing nodes such as placeholder | Conv2DTranspose |
| MatMul | - If the second input is a constant (const), it must be 2-dimensional<br>  - When both inputs are variable tensors, only INT8 symmetric quantization is supported<br>  - The input source of weight does not contain dynamically changing nodes such as placeholder | BatchMatMulV2 |
| AveragePool | global_pooling is false, and the shift-N operation is not supported | AvgPoolV2 |
| MaxPool | Only tensor quantization is performed | MaxPool, MaxPoolV3 |
| Add | Only tensor quantization is performed, and only single-input quantization is supported | Add |

<!-- npu="A3,910b,310b" id23 -->
For the MatMul quantization layer, in the scenario where both inputs are variable tensors, benefits can be obtained only on the following products. On other products, the precision decreases after quantization:

<!-- npu="A3" id24 -->
Atlas A3 products
<!-- end id24 -->

<!-- npu="910b" id25 -->
Atlas A2 products
<!-- end id25 -->

<!-- npu="310b" id26 -->
Atlas 200I/500 A2 inference product
<!-- end id26 -->
<!-- end id23 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/references/quantization_layers_constraints_res.md#id4 -->

## Weight-Only Quantization

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/references/quantization_layers_constraints_res.md#id2 -->

<!-- npu="910b,310p" id22 -->
**The weight-only quantization feature supports only the following product types:**

<!-- npu="310p" id13 -->
Atlas inference products
<!-- end id13 -->

<!-- npu="910b" id14 -->
Atlas A2 products
<!-- end id14 -->
<!-- end id22 -->

**Table 2**  Supported layers and constraints in the weight-only quantization scenario

|**Layer type defined by Ascend IR**|Weight-only quantization, channel_wise=true in weight ARQ|Weight-only quantization, asymmetric=true or false in weight ARQ|Both weight and data quantized, channel_wise=true in weight ARQ|Both weight and data quantized, asymmetric=true in weight ARQ|Constraint|
|--|--|--|--|--|--|
|MatMulV2|√|true|×|×|The input source of the second input does not contain dynamically changeable nodes such as placeholder.|
|BatchMatMulV2|√|true|×|×|The input source of the second input does not contain dynamically changeable nodes such as placeholder.|
|FFN|√|true and false|×|×|- The expert_tokens input of the FFN operator is not empty.<br>- The two weights of the FFN operator are Float16 constants.<br>- The antiquant_scale1, antiquant_scale2, antiquant_offset1, and antiquant_offset2 inputs of the FFN operator are empty.<br>- Weight sharing is not supported for weights.

Where:

- √ indicates support, and × indicates that quantization in this scenario will be abnormal.
- channel\_wise=true in weight ARQ: indicates that each channel is quantized independently with different quantization factors.
- asymmetric in weight ARQ
  - true: indicates that asymmetric quantization is used for weight quantization.
  - false: indicates that symmetric quantization is used for weight quantization.
  - true and false indicate that weight quantization supports both symmetric and asymmetric quantization.
<!-- npu="910b" id4 -->
- FFN operator quantization is supported only by Atlas A2  products.
<!-- end id4 -->
