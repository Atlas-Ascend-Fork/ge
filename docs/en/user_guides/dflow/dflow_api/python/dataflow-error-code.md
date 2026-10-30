# DataFlow Error Codes

<!-- md-trans-meta sourceCommit=41b60725da446bdf6e1a8c41b6b576f1fa8bd5f5 translatedAt=2026-10-28T13:07:30.157Z pushedAt=2026-10-29T03:01:14.756Z nl=2 -->

The DataFlow error codes and their meanings are as follows.

| Return Code | Meaning | Solution |
| --- | --- | --- |
| PARAM_INVALID=145000U | Invalid parameter validation | Locate the parameter that does not comply with the validation rule based on the specific API and logs, and modify it as instructed in the documentation or as prompted by the logs. |
| SHAPE_INVALID=145021U | Abnormal shape of the input tensor | Modify the input shape based on the logs. |
| DATATYPE_INVALID=145022U | Abnormal datatype of the input tensor | Modify the input datatype based on the logs. |
| NOT_INIT=145001U | DataFlow is not initialized | Call the [init](dataflow-init.md) API before using DataFlow APIs, as instructed in the documentation and sample code. |
| INNER_ERROR=545000U | Internal error in the Python layer | Troubleshoot the issue based on the logs, or contact an engineer for handling (after obtaining the logs, click [Support](https://www.hiascend.com/support) to contact technical support). |
| FAILED=0xFFFFFFFF | Internal error in the C++ layer | Troubleshoot the issue based on the logs, or contact an engineer for handling (after obtaining the logs, click [Support](https://www.hiascend.com/support) to contact technical support). |
| SUCCESS=0 | Execution succeeded | - |

For details about the logs involved in the error codes, see *[Log Reference](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/log_ref/README.md)*.
