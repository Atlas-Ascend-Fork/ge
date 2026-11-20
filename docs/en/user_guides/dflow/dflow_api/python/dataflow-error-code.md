# DataFlow Error Codes

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-11-16T05:11:12.402Z pushedAt=2026-11-16T07:49:08.978Z nl=2 -->

The DataFlow error codes and their meanings are as follows.

| Return Code | Meaning | Solution |
| --- | --- | --- |
| PARAM_INVALID=145000U | Invalid parameter validation | Locate the parameter that does not comply with the validation rule based on the specific API and logs, and modify it as instructed in the documentation or as prompted by the logs. |
| SHAPE_INVALID=145021U | Abnormal shape of the input tensor | Modify the input shape based on the logs. |
| DATATYPE_INVALID=145022U | Abnormal datatype of the input tensor | Modify the input datatype based on the logs. |
| NOT_INIT=145001U | DataFlow is not initialized | Call the [init](dataflow-init.md) API before using DataFlow APIs, as instructed in the documentation and sample code. |
| INNER_ERROR=545000U | Internal error in the Python layer | Troubleshoot the issue based on the logs, or contact engineers for handling (obtain the logs and click [Support](https://www.hiascend.com/support) to contact technical support).<br>For details about logs, see *Log Reference*. |
| FAILED=0xFFFFFFFF | Internal error in the C++ layer | Troubleshoot the issue based on the logs, or contact engineers for handling (obtain the logs and click [Support](https://www.hiascend.com/support) to contact technical support).<br>For details about logs, see *Log Reference*. |
| SUCCESS=0 | Execution succeeded | - |
