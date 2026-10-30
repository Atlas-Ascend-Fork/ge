# --deterministic

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T02:55:24.546Z pushedAt=2026-10-23T10:40:12.648Z -->

## Product Support

<!-- npu="950" id6 -->
- Ascend 950PR&950DT products: Supported
<!-- end id6 -->
<!-- npu="A3" id5 -->
- Atlas A3 products: Supported
<!-- end id5 -->
<!-- npu="910b" id4 -->
- Atlas A2 products: Supported
<!-- end id4 -->
<!-- npu="310b" id3 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id3 -->
<!-- npu="310p" id2 -->
- Atlas inference products: Supported
<!-- end id2 -->
<!-- npu="910" id1 -->
- Atlas training products: Supported
<!-- end id1 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--deterministic_res.md#id1 -->

## Feature Overview

Whether to enable deterministic computation.

By default, deterministic computation is disabled, and an operator may produce different results across multiple executions on the same hardware with the same input. This difference generally arises because the operator implementation involves asynchronous multi-threaded execution, which changes the order of floating-point accumulation. When deterministic computation is enabled, the operator produces the same output across multiple executions on the same hardware with the same input. However, enabling deterministic computation often slows down operator execution. When you find that a model produces different results across multiple executions, or when you are performing precision tuning, you can enable deterministic computation to facilitate model debugging and tuning.

## Parameter Values

- 0: (default) Deterministic computation is disabled.
- 1: Deterministic computation is enabled.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --deterministic=0 ...
```

## Dependency Constraints

In offline inference scenarios, it is recommended that you configure determinism and consistency through `--deterministic` and `--deterministic_level` during model conversion. When loading and executing an offline model through the `aclmdl` API, do not call `aclrtSetSysParamOpt` or `aclrtCtxSetSysParamOpt` to modify this configuration through `ACL_OPT_DETERMINISTIC`. Otherwise, the runtime configuration may become inconsistent with the configuration used during model conversion, and the expected determinism and consistency cannot be guaranteed. If you need to adjust this configuration, it is recommended that you reconvert the model using the target configuration.
