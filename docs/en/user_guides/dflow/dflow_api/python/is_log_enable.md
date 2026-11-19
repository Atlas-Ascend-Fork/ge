# is\_log\_enable

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T08:09:48.979Z pushedAt=2026-11-16T08:35:07.588Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 product series: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 product series: Supported
<!-- end id2 -->

## Function

Queries whether the log of the corresponding level and type is enabled.

## Prototype

```python
is_log_enable(self, log_type: fw.FlowFuncLogType, log_level: fw.FlowFuncLogLevel) -> bool
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| log_type | Input | Log type. The value can be:<br><br>  - DEBUG_LOG<br>  - RUN_LOG |
| log_level | Input | Log level. The value can be:<br><br>  - DEBUG<br>  - INFO<br>  - WARN<br>  - ERROR |

## Return Value

- True: Enabled
- False: Disabled

## Exception Handling

None.

## Constraints

None.
