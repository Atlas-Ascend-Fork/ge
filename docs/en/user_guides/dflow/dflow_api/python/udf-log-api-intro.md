# UDF Log API Overview

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T09:42:53.472Z pushedAt=2026-11-17T03:39:12.071Z nl=2 -->

UDF Python provides log record APIs. Import the flow\_func module before use. Use the logger object defined in the module to call the log APIs of different levels encapsulated by the logger object.

**Table 1** Log classification

| Log Type | Scenario | Log Level | Corresponding Log Macro |
| --- | --- | --- | --- |
| Run log | Runtime logs need to be recorded for abnormal states and abnormal actions during system running, key events during system process running, and information related to system resource usage. | ERROR/INFO | [Error-Level Log Macro of Run Logs](run-log-error-level-log-macro.md)/[Info-Level Log Macro of Run Logs](run-log-info-level-log-macro.md) |
| Debug log | The following scenarios (including but not limited to these scenarios) need to be recorded as debug-level logs:<br>- Entries and exits of all calls, such as API calls and function calls<br>- Operation entries and preset conditions<br>- Timer start and timeout<br>- Before and after state setting and state transition condition judgment<br>- Service-related resource statistics, service processing entries and exits, and performance calculation statistics<br>- All processing failures and exceptions<br>Debug-level logs record code-level information and are used by developers to locate issues. | ERROR/WARN/INFO/DEBUG | [Error-Level Log Macro of Debug Logs](debug-log-error-level-log-macro.md)/[Warn-Level Log Macro of Debug Logs](debug-log-warn-level-log-macro.md)/[Error-Level Log Macro of Debug Logs](debug-log-error-level-log-macro.md)/[Debug-Level Log Macro of Debug Logs](debug-log-debug-level-log-macro.md) |

Log usage example:

```python
import dataflow.flow_func as ff
ff.logger.info("This is a test info log :%s %d %f.", "test_str", 100, 0.1)
```

For log level modification, see the usage of ASCEND\_GLOBAL\_LOG\_LEVEL and ASCEND\_MODULE\_LOG\_LEVEL in the *Environment Variable Reference*. The module corresponding to user UDF logs is APP, and the APP log level can be controlled separately by module.

For example, to enable Info-level logs of a custom UDF, the following command can be used to enable the Info-level logs of the APP module separately.

```sh
export ASCEND_MODULE_LOG_LEVEL=APP=1
```

The following command can also be used to enable Info-level logs of all modules.

```sh
export ASCEND_GLOBAL_LOG_LEVEL=1
```

During UDF execution, flow control is applied to APP module logs to prevent excessive logs from affecting framework logs and execution performance. The restriction rules are as follows:

**Table 2** Restriction rules

| API Type | Debug Log | Debug Log | Debug Log | Debug Log | Runtime Log | Runtime Log |
| --- | --- | --- | --- | --- | --- | --- |
| Level | debug | info | warn | error | run_info | run_error |
| DEBUG | unlimited | unlimited | unlimited | unlimited | 50/400 | 50/400 |
| INFO | not print | 100/1000 | 100/1000 | 100/1000 | 50/400 | 50/400 |
| WARN | not print | not print | 50/400 | 50/400 | 50/400 | 50/400 |
| ERROR | not print | not print | not print | 50/400 | 50/400 | 50/400 |

> [!NOTE]
> For each independent process, A logs per second, with a cumulative upper limit of B, shown as \[A/B\] in the table above. The cumulative upper limit refers to the maximum number of logs that can be output instantly after the process has not printed logs for a period of time.
