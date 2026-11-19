# UDF Log API Overview

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T04:15:11.443Z pushedAt=2026-11-16T06:29:03.212Z nl=2 -->

provides log APIs to facilitate log recording during flow func development. The log-related scenarios are as follows.

**Table 1** Log classification

| Log Type | Scenario | Log Level | Corresponding Log Macro |
| --- | --- | --- | --- |
| Runtime Log | Runtime logs need to be recorded for abnormal states and abnormal actions during system running, key events during system process running, and information related to system resource usage. | ERROR/INFO | [Error-Level Log Macro of Run Logs](run-log-error-level-log-macro.md)/[Info-Level Log Macro of Run Logs](run-log-info-level-log-macro.md) |
| Debug Log | The following scenarios (including but not limited to these scenarios) need to be recorded as debug-level logs:<br>- Entries and exits of all calls, such as API calls and function calls<br>- Operation entries and preset conditions<br>- Timer start and timeout<br>- Before and after state setting and state transition condition judgment<br>- Service-related resource statistics, service processing entries and exits, and performance calculation statistics<br>- All processing failures and exceptions<br>Debug-level logs record code-level information and are used by developers to locate issues. | ERROR/WARN/INFO/DEBUG | [Error-Level Log Macro of Debug Logs](debug-log-error-level-log-macro.md)/[Warn-Level Log Macro of Debug Logs](debug-log-warn-level-log-macro.md)/[Info-Level Log Macro of Debug Logs](debug-log-info-level-log-macro.md)/[Debug-Level Log Macro of Debug Logs](debug-log-debug-level-log-macro.md) |
