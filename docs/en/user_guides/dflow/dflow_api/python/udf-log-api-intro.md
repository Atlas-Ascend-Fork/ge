# UDF Log API Overview

<!-- md-trans-meta sourceCommit=41b60725da446bdf6e1a8c41b6b576f1fa8bd5f5 translatedAt=2026-10-28T12:46:24.693Z pushedAt=2026-10-29T03:29:53.867Z nl=2 -->

UDF Python provides log recording APIs. To use them, import the flow\_func module, use the logger object defined in it, and call the log APIs of different levels encapsulated by the logger object.

**Table 1** Log classification

| Log Type | Scenario | Log Level | Corresponding Log Macro |
| --- | --- | --- | --- |
| Runtime Log | Runtime logs need to be recorded for abnormal states and abnormal actions during system running, key events during system process running, and information related to system resource usage. | ERROR/INFO | [Runtime Log Error Level Log Macro](run-log-error-level-log-macro.md)/[Runtime Log Info Level Log Macro](run-log-info-level-log-macro.md) |
| Debug Log | The following scenarios (including but not limited to these scenarios) need to be recorded as debug-level logs:<br>- Entries and exits of all calls, such as API calls and function calls<br>- Operation entries and preset conditions<br>- Timer start and timeout<br>- Before and after state setting and state transition condition judgment<br>- Service-related resource statistics, service processing entries and exits, and performance calculation statistics<br>- All processing failures and exceptions<br>Debug-level logs record code-level information and are used by developers to locate issues. | ERROR/WARN/INFO/DEBUG | [Debug Log Error Level Log Macro](debug-log-error-level-log-macro.md)/[Debug Log Warn Level Log Macro](debug-log-warn-level-log-macro.md)/[Debug Log Error Level Log Macro](debug-log-error-level-log-macro.md)/[Debug Log Debug Level Log Macro](debug-log-debug-level-log-macro.md) |

Log usage example:

```python
import dataflow.flow_func as ff
ff.logger.info("This is a test info log :%s %d %f.", "test_str", 100, 0.1)
```

For how to modify the log level, see the usage of ASCEND\_GLOBAL\_LOG\_LEVEL and ASCEND\_MODULE\_LOG\_LEVEL in *[Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md)*. The module corresponding to user UDF logs is APP, and the APP log level can be controlled separately by module.

For example, to enable Info-level logs for a custom UDF, run the following command to enable the info-level logs of the APP module separately:

```sh
export ASCEND_MODULE_LOG_LEVEL=APP=1
```

You can also run the following command to enable info-level logs for all modules:

```sh
export ASCEND_GLOBAL_LOG_LEVEL=1
```

During UDF execution, rate limiting is applied to APP module logs to prevent excessive logs from affecting framework logs and execution performance. The limiting rules are as follows:

**Table 2** Rate limiting rules

| Interface Type | Debug Log | Debug Log | Debug Log | Debug Log | Runtime Log | Runtime Log |
| --- | --- | --- | --- | --- | --- | --- |
| Level | debug | info | warn | error | run_info | run_error |
| DEBUG | No Rate Limit | No Rate Limit | No Rate Limit | No Rate Limit | 50/400 | 50/400 |
| INFO | Not printed | 100/1000 | 100/1000 | 100/1000 | 50/400 | 50/400 |
| WARN | Not printed | Not printed | 50/400 | 50/400 | 50/400 | 50/400 |
| ERROR | Not printed | Not printed | Not printed | 50/400 | 50/400 | 50/400 |

> [!NOTE] Note
> For each independent process, A logs per second with a cumulative upper limit of B, shown as \[A/B\] in the table above. The cumulative upper limit refers to the maximum number of logs that can be output instantly after the process has not printed logs for a period of time.
