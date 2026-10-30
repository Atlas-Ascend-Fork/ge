# --ac\_parallel\_enable

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T02:44:09.510Z pushedAt=2026-10-22T04:03:52.031Z -->

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

<!-- npu="310b" id5 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id5 -->

<!-- npu="310p" id6 -->
- Atlas inference products: Supported
<!-- end id6 -->

<!-- npu="910" id7 -->
- Atlas training products: Supported
<!-- end id7 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--ac_parallel_enable_res.md#id1 -->

## Function Description

<!-- npu="950,A3,910b,910,310p,310b" id8 -->
In a dynamic shape graph, whether AI CPU operators and AI Core operators are allowed to run in parallel.

In a dynamic shape graph, when the switch is enabled, the system automatically identifies AI CPU operators that can run concurrently with AI Core operators, and dispatches operators of different engines to different streams to implement parallelism among multiple engines, thereby improving resource utilization and dynamic shape execution performance.
<!-- end id8 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--ac_parallel_enable_res.md#id2 -->

## Parameter Values

<!-- npu="950,A3,910b,910,310p,310b" id9 -->
- 1: Allows parallel execution between AI CPU and AI Core operators.
<!-- end id9 -->
<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--ac_parallel_enable_res.md#id3 -->
- 0: (Default) AI CPU operators are not separately offloaded.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --ac_parallel_enable=1 ...
```
