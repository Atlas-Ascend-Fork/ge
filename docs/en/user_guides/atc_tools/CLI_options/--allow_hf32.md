# --allow\_hf32

<!-- md-trans-meta sourceCommit=607806abd035eaf2ab464a02634ae7b347df3984 translatedAt=2026-10-27T14:07:38.311Z pushedAt=2026-10-27T14:17:44.554Z nl=2 -->

## Product Support

<!-- npu="950" id7 -->
- Ascend 950PR&950DT products: Supported
<!-- end id7 -->
<!-- npu="A3" id6 -->
- Atlas A3 products: Supported
<!-- end id6 -->
<!-- npu="910b" id5 -->
- Atlas A2 products: Supported
<!-- end id5 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id4 -->
<!-- npu="310p" id3 -->
- Atlas inference products: Not supported
<!-- end id3 -->
<!-- npu="910" id2 -->
- Atlas training products: Not supported
<!-- end id2 -->

## Feature Description

Specifies whether to enable the function of automatically replacing the float32 data type with HF32. In the current version, this parameter takes effect only for Conv class operators and Matmul class operators.

HF32 is a single-precision floating-point type launched by Ascend specifically for internal operator computation. The following figure compares HF32 with other common data types. As shown, HF32 supports the same value range as float32, but its mantissa precision (11 bits) is close to that of FP16 (10 bits). By reducing precision, HF32 single-precision data type replaces the original float32 single-precision data type, which greatly reduces the space occupied by data and improves performance.

**Figure 1**  HF32 compared with other data types
![Diagram](../figures/hf32_data_type_comparison.png "HF32 compared with other data types")

## Related Parameters

- For the same operator, if [--op\_precision\_mode](--op_precision_mode.md) is used to configure enable\_hi\_float\_32\_execution or enable\_float\_32\_execution, it is not recommended to use it together with [--allow\_hf32](--allow_hf32.md) in this scenario. If they are used together, the priority is as follows:

    op\_precision\_mode\(ByNodeName, set the precision mode by node name \) \> allow\_hf32 \> op\_precision\_mode\(ByOpType, set the precision mode by operator type \)

- Because --allow\_hf32 uses HF32 to automatically replace float32, for this parameter to take effect, the input or output type of the involved operators must be float32. Because the default value of the [--precision\_mode\_v2](--precision_mode_v2.md) parameter is fp16, operators whose type is float32 in the original network model are forcibly converted to float16. In this scenario, the --allow\_hf32 parameter does not take effect. It is recommended to change the value of the [--precision\_mode\_v2](--precision_mode_v2.md) parameter to **origin** (the default value of the [--precision\_mode](--precision_mode.md) parameter is force\_fp16; it is recommended to change it to **must\_keep\_origin\_dtype** or **force\_fp32**).

## Parameter Values

**Parameter values:**

- true: For Conv class operators and Matmul class operators, enables the function of automatically converting the FP32 data type to the HF32 data type.

    For the specific operators involved, see opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/allow\_hf32\_matmul\_**t**\_conv\_**t**.ini in the file storage path after CANN software installation. This file does not support user modification.

- false: For Conv class operators and Matmul class operators, disables the function of automatically converting the FP32 data type to the HF32 data type.

    For the specific operators involved, see opp/built-in/op\_impl/ai\_core/tbe/impl\_mode/allow\_hf32\_matmul\_**f**\_conv\_**f**.ini in the file storage path after CANN software installation. This file does not support user modification.

**Parameter default value:**
For Conv class operators, the conversion from FP32 to HF32 is enabled by default. For Matmul class operators, this conversion function is disabled by default.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --allow_hf32=true ...
```

If “ge.exec.allow\_hf32” can be found in the model conversion log, the parameter setting has taken effect.

## Dependency Constraints

None.
