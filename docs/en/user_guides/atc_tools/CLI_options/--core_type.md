# --core\_type

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T02:51:16.808Z pushedAt=2026-10-29T08:18:24.715Z nl=2 -->

## Product Support

<!-- npu="950" id6 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id6 -->
<!-- npu="A3" id5 -->
- Atlas A3 products: Not supported
<!-- end id5 -->
<!-- npu="910b" id4 -->
- Atlas A2 products: Not supported
<!-- end id4 -->
<!-- npu="310b" id3 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id3 -->
<!-- npu="310p" id2 -->
- Atlas inference products: Supported
<!-- end id2 -->
<!-- npu="910" id1 -->
- Atlas training products: Not supported
<!-- end id1 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--core_type_res.md#id1 -->

## Function Description

Sets the Core type used for model compilation.

<!-- npu="310p" id8 -->
This parameter is supported only on Atlas inference products. On other products, model conversion succeeds after this parameter is configured, but the function does not take effect.
<!-- end id8 -->

## Related Parameters

None.

## Parameter Values

**Parameter values:**

- VectorCore
- AiCore, which is the default value

**Parameter value constraints:**
If the network model contains Cube operators, only AiCore can be used.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --core_type=VectorCore ...
```

## Dependency Constraints

None.
