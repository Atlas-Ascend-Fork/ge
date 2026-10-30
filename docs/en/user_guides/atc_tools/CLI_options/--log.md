# --log

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-22T03:17:28.758Z pushedAt=2026-10-26T03:50:10.693Z -->

## Supported Products

Supported on all chips

## Description

Sets the log level during ATC model conversion.

## Related Parameters

None.

## Parameter Values

**Debug Log** supports the following levels:

- debug: Outputs debug/info/warning/error-level debug log information.
- info: Outputs info/warning/error-level debug log information.
- warning: Outputs warning/error-level debug log information.
- error: Outputs error-level debug log information.
- null: (Default) Does not output debug logs.

**Run Log** outputs info/warning/error/event-level logs by default and does not support level adjustment. **Security Log** outputs debug/info/warning/error-level logs by default and does not support level adjustment.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --log=debug ...
```

If model conversion fails, you can locate the problem by analyzing logs. The log format is as follows. For more log information, see *Log Reference*.

```console
[Level] ModuleName(PID,PName):DateTimeMS [FileName:LineNumber]LogContent
```

The fields are described as follows:

**Table 1**  Log field description

| Field | Description |
| --- | --- |
| Level | Log level. The debug log has four levels: ERROR, WARNING, INFO, and DEBUG. |
| ModuleName | Name of the module that generates the log. |
| PID | Process ID. |
| PName | Process name. |
| DateTimeMS | Time when the log is printed, in the format of yyyy-mm-dd-hh:mm:ss.SSS.SSS. |
| FileName:LineNumber | File that calls the log printing API and the corresponding line number. |
| LogContent | Specific log content of each module. |

The following is an example:

```console
[INFO] FE(30741,atc.bin):2021-12-09-16:10:22.539.141 [fe_type_utils.cc:52]30741 GetRealPath:"path /usr/local/Ascend/opp/built-in/op_impl/ai_core/tbe/config/ascendxxx is not exist."
[WARNING] FE(30741,atc.bin):2021-12-09-16:10:22.539.146 [sub_op_info_store.cc:52]30741 Initialize:"The config file[/usr/local/Ascend/opp/built-in/op_impl/ai_core/tbe/config/ascendxxx] of op information library[tbe-builtin] is not existed. "
[ERROR] GE(30741,atc.bin):2021-12-09-16:10:22.539.201 [error_manager.cc:263]30741 ReportErrMessage: [INIT][OPS_KER][Report][Error]error_code: W21000, arg path is not existed in map
```

Problem locating ideas:

**Table 2**  Problem locating ideas

| Field | Description | Solution |
| --- | --- | --- |
| GE | GE graph compilation or verification problem. | For verification errors, a clear error cause is usually provided. In this case, modify the parameters used for model conversion accordingly to meet the relevant requirements. |
| FE | Operator fusion problem. | None. |
| TEFUSION | - Operator precompilation/compilation problem.<br>  - Fusion operator compilation problem. | Common error messages and solutions:<br><br>  1. ModuleNotFoundError: No module named 'decorator'Solution: Install the pip package as prompted.<br>  2. ModuleNotFoundError: No module named 'te'Solution: When installing the software package where the ATC tool resides, the --pylocal parameter is not used in the installation command. It is recommended to use this parameter to reinstall the corresponding software package. |
| TBE | Operator compilation problem. | None. |

## Dependencies

<!-- npu="950,A3,910b,910,310p,310b" id2 -->
- **Log Persistence**:

    During the execution of the atc command, logs are persisted to the following paths by default:

  - $HOME/ascend/log/**debug**/plog/plog-_pid_\_\*.log: Debug Log.

    In the Debug Log scenario, because the default value of [--log](--log.md) is null, that is, no logs are output, if the preceding path contains log information, it is log information from sources other than the atc process, such as Python-related information. To make the logs reflect atc process-related information, set [--log](--log.md) to a value other than null.

  - $HOME/ascend/log/**run**/plog/plog-_pid_\_\*.log: Runtime Log.
  - $HOME/ascend/log/**security**/plog/plog-_pid_\_\*.log: Security Log.

    pid indicates the process ID, and “\*” indicates the timestamp when the log file was created.
  <!-- end id2 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--log_res.md#id1 -->

<!-- npu="950,A3,910b,910,310p,310b" id3 -->
- **Log Display**:

    During the execution of the atc command, logs are not displayed on the screen by default. To display them on the screen, do the following:

  - Set the screen-printing environment variable in the current window where the atc command is executed:

    ```bash
     export ASCEND_SLOG_PRINT_TO_STDOUT=1
    ```

  - In the atc model conversion command, set the [--log](--log.md) parameter (it cannot be set to null).

    For more information about logs, see *[Log Reference](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/log_ref/README.md)*.
<!-- end id3 -->

- **Log redirection**:

    If you do not want logs to be written to disk but redirected to a file, set the screen-printing environment variable described above before model conversion, and set the [--log](--log.md) parameter in the atc command (it cannot be set to null). The following is an example. Replace xxx with the actual parameters:

    ```bash
    atc xxx --log=debug >log.txt
    ```
