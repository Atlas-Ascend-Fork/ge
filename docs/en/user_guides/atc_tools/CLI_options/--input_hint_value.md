# --input_hint_value

<!-- md-trans-meta sourceCommit=48c4eb84435d0484d9b11b23287fbcd9a5cb6d3e translatedAt=2026-11-16T14:04:11.061Z pushedAt=2026-11-17T09:06:58.014Z nl=2 -->

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

Sets the value hint information for the input data of a dynamic shape model.

When the input data of the original model has a dynamic shape, the specific input value cannot be obtained during model conversion. This parameter specifies the value hint information by the index of the input node. During model conversion, this value is used for compile-time symbol value derivation and guard generation. During actual model execution, the model still runs based on the real input data at runtime.

This parameter only participates in compile-time symbolization and does not change the shape or value of the runtime input. It does not take effect on inputs that do not support value symbolization or are not host placement.

## Related Parameters

It can be used together with [--input_hint_shape](--input_hint_shape.md) to provide shape hints and value hints respectively.

This parameter is not supported in OM2 mode.

## Parameter Values

**Parameter value:**
A combination of the input node index and the value hint list, for example, `"0:[1,2];1:[3,4]"`.

**Parameter value format:**

- The format of a single input is `index:[v0,v1,...]`, where index is the sequence number of the input node (counting from 0). Multiple inputs are separated by semicolons.
- The specified parameter must be enclosed in double quotation marks.

**Parameter value constraints:**

- The value hint list must be enclosed in square brackets `[]`. The value of each element is an integer greater than or equal to 0.
- index must be an integer greater than or equal to 0, and cannot be repeated within the same parameter.
- The number of elements in the value hint must not exceed the number of elements in the corresponding input tensor. If it exceeds, the hint is ignored.
- The value hint takes effect only on host inputs that require value symbolization.

**Default value:**
None. When it is not set, this function does not take effect.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --input_hint_shape="0:[2];1:[2]" \
    --input_hint_value="0:[6,3];1:[1,2]" ...
```
