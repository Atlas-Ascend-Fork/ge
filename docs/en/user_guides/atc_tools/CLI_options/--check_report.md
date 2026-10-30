# --check\_report

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-22T02:48:01.789Z pushedAt=2026-10-23T04:01:19.796Z -->

## Product Support

<!-- npu="950,A3,910b,910,310p,310b" id2 -->
All chips are supported.
<!-- end id2 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--check_report_res.md#id1 -->

## Function Description

Used to configure the save file path and file name of the pre-check result.

## Related Parameters

[--mode](--mode.md): When --mode=0 and graph parsing fails, or when --mode=3 performs only pre-check, this parameter specifies the save path of the pre-check result file.

## Parameter Value

**Parameter value:**
Path and file name of the pre-check result file.

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Default value:**
check_result.json is generated in the current directory where the atc command is executed.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --check_report=$HOME/module/out/check_result.json ...
```

## Constraints

The storage path of the pre-check result file. In addition to setting it through the [--check\_report](--check_report.md) parameter, you can also configure the environment variable ASCEND\_WORK\_PATH. The priority of these methods is as follows: the configuration parameter "--check\_report" > the environment variable ASCEND\_WORK\_PATH > the default storage path (the current directory where the atc command is executed).

For details about the environment variable ASCEND\_WORK\_PATH, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).
