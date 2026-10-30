# --enable\_single\_stream

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:02:14.773Z pushedAt=2026-10-27T07:35:19.654Z -->

## Product Support

<!-- npu="950" id6 -->
- Ascend 950PR&amp;950DT products: Supported
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

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--enable_single_stream_res.md#id1 -->

## Function Description

In static shape scenarios, whether to enable sequential single-stream serial execution of model inference.

A stream is used to maintain the execution order of asynchronous operations, ensuring that they are executed on the Device in the order in which the code is called in the application.

## Related Parameters

None.

## Parameter Values

**Parameter values:**

- true: enables the feature. Model inference is executed serially in a single stream.
- false: (default) disables the feature. Model inference is executed in parallel across multiple streams.

**Parameter value constraints:**

When the model contains the Cmo operator and the following control operators, the single-stream feature cannot be used, and only the default value false is allowed.

- Merge
- Switch
- Enter
- RefEnter

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --enable_single_stream=true ...
```

## Usage Constraints

None.
