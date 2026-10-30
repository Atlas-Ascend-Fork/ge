# --deterministic\_level

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T02:54:22.330Z pushedAt=2026-10-23T10:07:45.149Z -->

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

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--deterministic_level_res.md#id1 -->

## Feature Overview

**This parameter is a debugging parameter and is not supported in production environments. It will be released as a formal feature in a later version.**

Deterministic computing level.

By default, the deterministic level is 0, which means deterministic computing is disabled, and the [--deterministic](--deterministic.md) option that controls whether to enable deterministic computing must also be 0. To enable deterministic computing, set the deterministic level to 1 and set the --deterministic option to 1. To enable strong consistent computing, set the level to 2 and set the --deterministic option to 1. To enable batch consistent computing, set the level to 3 and set the --deterministic option to 1.

When strong consistency computing is enabled (--deterministic\_level=2), the computation result is deterministic, and multiple executions produce the same output. In addition, the computation result is independent of the data location. For example, during matrix multiplication, the accumulation order of different rows may differ, which may cause slight differences in the computation results of the same data in different rows. However, when strong consistent computing is enabled, the computation results remain consistent across different rows as long as the input is the same.

By default, strong consistent computing is not enabled. In this default mode, the same data in different rows may produce inconsistent computation results.

For performance reasons, it is generally recommended not to enable strong consistent computing, because it reduces the computing speed of operators and affects overall efficiency. Enable this feature only in the following scenarios: when you need to strictly guarantee result consistency for the same data at different locations, or when you are performing precision tuning and debugging on a model to optimize overall performance.

## Related Parameters

Must be used together with the [--deterministic](--deterministic.md) parameter.

## Parameter Values

- 0: (default) deterministic computing is disabled.
- 1: deterministic computing is enabled.
- 2: strong consistent computing is enabled.
- 3: batch consistent computing is enabled.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --deterministic=0  --deterministic_level=0 ...
```

## Dependencies and Constraints

In offline inference scenarios, you are advised to set the deterministic and consistent configurations through `--deterministic` and `--deterministic_level` during model conversion. When loading and executing an offline model through the `aclmdl` API, do not call `aclrtSetSysParamOpt` or `aclrtCtxSetSysParamOpt` to modify this configuration through `ACL_OPT_DETERMINISTIC`. Otherwise, the runtime configuration may be inconsistent with the configuration used during model conversion, and the expected deterministic and consistent effects cannot be guaranteed. If you need to adjust this configuration, you are advised to reconvert the model with the target configuration.
