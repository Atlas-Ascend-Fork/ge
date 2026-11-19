# --input\_hint\_shape

<!-- md-trans-meta sourceCommit=48c4eb84435d0484d9b11b23287fbcd9a5cb6d3e translatedAt=2026-11-16T14:03:21.733Z pushedAt=2026-11-17T09:06:37.047Z nl=2 -->

## Product Support

<!-- npu="950" id2 -->
- Ascend 950PR&950DT products: Supported
<!-- end id2 -->

<!-- npu="A3" id3 -->
- Atlas A3 products: Supported
<!-- end id3 -->

<!-- npu="910b" id4 -->
- Atlas A2 products: Supported
<!-- end id4 -->

## Function Description

Sets the shape prompt message of the model input data for a dynamic shape model.

When the shape of the original model input data is not fixed (dynamic shape), the specific shape of the input cannot be obtained during model conversion. This parameter specifies the shape prompt message by the index of the input node. During model conversion, this shape is used as the prompt shape corresponding to the input and participates in shape derivation at compile time. When the model is actually executed, the actual shape of the input at runtime is still used.

## Related Parameters

- This parameter cannot be used together with [--dynamic\_batch\_size](--dynamic_batch_size.md), [--dynamic\_image\_size](--dynamic_image_size.md), or [--dynamic\_dims](--dynamic_dims.md). If they are set at the same time, model conversion reports an error.
- When [--mode](--mode.md) is set to 7, this parameter is not supported.

## Parameter Values

**Parameter value:**
A combination of the index of the input node and the shape prompt message, for example, "0:[16,32,208,208];1:[16,3,224,224]".

**Parameter value format:**

- The format of a single input is `index:[n,c,h,w]`, where index is the sequence number of the input node (counting from 0). Multiple inputs are separated by semicolons in English.
- The specified parameter must be enclosed in double quotation marks.

**Parameter value constraints:**

- The shape prompt message must be enclosed in English square brackets \[\], and the value of each dimension is an integer greater than or equal to 0.
- The number of dimensions of the shape prompt message must be consistent with the actual number of dimensions of the corresponding input. Otherwise, the prompt message does not take effect at compile time.

**Parameter default value:**
None. If this parameter is not set, this function does not take effect.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --input_hint_shape="0:[16,32,208,208];1:[16,3,224,224]" ...
```
