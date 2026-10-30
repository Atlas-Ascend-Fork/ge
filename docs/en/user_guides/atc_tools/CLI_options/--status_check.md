# --status\_check

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:43:50.407Z pushedAt=2026-10-27T13:17:53.900Z -->

## Product Support

<!-- npu="950" id2 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id2 -->

<!-- npu="A3" id3 -->
- Atlas A3 products: Not supported
<!-- end id3 -->

<!-- npu="910b" id4 -->
- Atlas A2 products: Not supported
<!-- end id4 -->

<!-- npu="310b" id5 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id5 -->

<!-- npu="310p" id6 -->
- Atlas inference products: Supported
<!-- end id6 -->

<!-- npu="910" id7 -->
- Atlas training products: Supported
<!-- end id7 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--status_check_res.md#id1 -->

## Function Description

Controls whether overflow detection logic is inserted when compiling operators.

If the model computation precision is abnormal and is suspected to be caused by operator overflow, you can enable this parameter during compilation to add overflow detection logic, and then recompile the model for troubleshooting.

## Related Parameters

- When using this parameter, it is recommended to use it together with the [--op\_debug\_level](--op_debug_level.md) parameter. In this way, you can check whether overflow detection logic has been added in the generated operator \*.cce file. The following is a code example with overflow detection logic added:

    ```c++
      if (status_overflow[0]) {
        xxxxxx
    }
    ```

## Parameter Values

- 0: (default) Overflow detection is disabled, and no overflow detection logic is inserted during operator compilation.
- 1: Overflow detection is enabled, and overflow check logic is inserted during operator compilation.

## Example

```bash
atc --status_check=1 ...
```

## Usage Constraints

Using the [--status\_check](--status_check.md) parameter only adds overflow detection logic to the operator \*.cce file generated after model compilation. To view which operators overflow, you need to use the **aclInit** API provided during model inference and enable the “dump\_debug” switch in the JSON configuration file passed to this API. For details about the API, see the "Initialization and Deinitialization \> aclInit" section in *[Runtime API](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md)*.
