# --multi\_stream\_parallel\_mode

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:21:13.536Z pushedAt=2026-10-26T09:31:32.087Z -->

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

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--multi_stream_parallel_mode_res.md#id1 -->

## Feature Description

**This is an extended parameter for debugging. It is not supported in production environments and will be released as a formal feature in later versions.**

This parameter applies to static and dynamic shape graph scenarios. Developers can configure this parameter to control the automatic allocation policy of the multi-stream parallel mode, thereby improving graph execution performance.

## Related Parameters

- This parameter is mutually exclusive with [--enable\_single\_stream](--enable_single_stream.md) and cannot be enabled at the same time. If both are enabled, compilation reports an error and terminates.

- In dynamic shape scenarios, you must first enable dynamic shape multi-stream through the environment variable [ENABLE\_DYNAMIC\_SHAPE\_MULTI\_STREAM](../../env_vars/ENABLE_DYNAMIC_SHAPE_MULTI_STREAM.md) before this parameter can be used to select the multi-stream parallel algorithm. Dynamic shape multi-stream cannot be enabled independently.

## Parameter Values

**Parameter values:**

- cv: Enables parallel execution of Cube operators and Vector operators.
- LoadBalance:N: Load balancing algorithm, which evenly distributes all operators across N streams for execution.
- MainStream:N: Main stream algorithm, in which serial operators are executed on the main stream, and other parallelizable operators are executed on other streams.
- Empty string (default value): No multi-stream parallel optimization is enabled.

**Parameter value constraints:**

N is the maximum number of streams, a positive integer in the range of \[1,64\]. A value outside this range causes compilation to fail.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --multi_stream_parallel_mode=LoadBalance:8 ...
```

## Constraints

- This parameter is applicable only to recommended network types.
- In dynamic shape multi-stream scenarios, to use this feature, you need to first enable dynamic shape multi-stream through [ENABLE\_DYNAMIC\_SHAPE\_MULTI\_STREAM](../../../user_guides/env_vars/ENABLE_DYNAMIC_SHAPE_MULTI_STREAM.md) and then configure this parameter.
