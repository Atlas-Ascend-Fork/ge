# Debugging and Common Issues

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:34:07.373Z pushedAt=2026-10-28T03:30:34.494Z nl=2 -->

## Development Check Items (by Graph Integration Stage)

- **Stage 1-2 (Definition and Bridging):**
  - The operator type name, registered class name, and graph-side op type are consistent.
  - PyTorch scenario: TORCH\_LIBRARY schema is consistent with the REG\_OP input and output.
  - The GE op type in the converter is consistent with the REG\_AUTO\_MAPPING\_OP registered name.

- **Stage 3 (Graph Integration):**
  - PyTorch: the converter is registered, and torchair.ge\_mode\(\) takes effect.
  - ONNX: the parsing plugin is loaded (before ATC compilation), and the OriginOpType format is domain::version::OpType.

- **Stage 4-5 (Compilation and Execution):**
  - ASCEND\_CUSTOM\_OPP\_PATH points to the root directory of the OPP package.
  - op\_graph/lib/<os\>/<arch\>/ contains .so.
  - The kernel binary/source code path does not depend on the temporary working directory.
  - In the offline OM scenario, verify that soc\_version is consistent with the actual environment.

## Common Error Troubleshooting

| Symptom | Possible Cause | Troubleshooting Method |
| --- | --- | --- |
| GE cannot find the custom operator | ASCEND_CUSTOM_OPP_PATH is not configured or the path is incorrect | Check the environment variable and confirm that the .so file exists under op_graph/lib/\<os>/\<arch>/. |
| Execute returns null input | The input index does not match the REG_OP definition | Verify the INPUT order in REG_OP and the GetInputTensor index in Execute. |
| The shape is empty at compile time | The input shape is not ready during the Compile phase | Check whether GetInputTensor is correctly read in Compile. |
| Execute fails after the OM is loaded | Deserialize does not correctly restore the binary | Check whether the buffer formats in Serialize/Deserialize are symmetric. |
| An error is reported in PyTorch graph mode | The converter is not registered or the op type name does not match | Confirm that the GE op type in register_fx_node_ge_converter is consistent with REG_AUTO_MAPPING_OP. |
| The result value is incorrect (the process runs but the output is wrong) | The launch parameter unit is incorrect (bytes and element counts are mixed) | Use GetShapeSize() for the element count instead of GetSize(); verify the block configuration. |
| The result value is incorrect (broadcast-related) | The broadcast implementation in InferShape is invalid | Follow the standard broadcast rules (tail alignment, expansion allowed only when the dimension is 1), and return GRAPH_FAILED when invalid. |
| PARAM_INVALID is reported during ONNX-to-ATC conversion | The parser does not recognize the op type | Confirm that the parsing plugin is loaded and that the domain/version of OriginOpType matches the model. |
| "Symbol not found/signature mismatch" is reported during build | The CANN/GE version is inconsistent with the documentation example | Verify the version compatibility and use the header files of the target version as the reference. |

## Logs and Errors

- **CANN runtime logs (plog)**: The log level is controlled through environment variables to troubleshoot underlying issues such as GE loading .so files, instantiating operators, and callback invocation failures.

    ```bash
    export ASCEND_GLOBAL_LOG_LEVEL=1   # 0=DEBUG 1=INFO 2=WARNING 3=ERROR
    export ASCEND_SLOG_PRINT_TO_STDOUT=1
    ```

- **Error code ranges**: Runtime error codes take forms such as 161xxx (parameter/environment class) and 507xxx (aicore timeout class). The error code format is \[ error code \]\[ module \]\[ submodule \]\[ error cause \]. When locating an issue, first determine which stage the error code belongs to.
- **Compile-time logs**: When RTC compilation fails, the logs should contain detailed kernel source code compilation errors.

For the complete error code descriptions, see [Troubleshooting](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/troubleshooting/00_troubleshooting.md).

## How to Further Locate the Issue

1. **Narrow down by stage**: Determine at which stage the error occurs: "definition/bridging → graph insertion → compilation → execution". For the check items, see [Development Check Items (by Graph Integration Stage)](#development-check-items-by-graph-integration-stage).
2. **Verify three-way consistency**: Focus on checking whether REG\_OP, REG\_AUTO\_MAPPING\_OP, and the op type on the graph construction side match exactly. This is the most frequent root cause of issues.
3. **Check logs**: Enable plog (see [Logs and Errors](#logs-and-errors)) and reproduce the issue, focusing on the logs and error codes of the GE loading process and the callback invocation lines.
4. **Minimize reproduction**: Start from the Add example in the quick start and gradually replace it with your own operator, avoiding introducing multiple unknown factors at once.
5. **If the issue cannot be located**: Report the issue to the community or official support channel together with the environment version information, the CANN version, and the complete error logs.
