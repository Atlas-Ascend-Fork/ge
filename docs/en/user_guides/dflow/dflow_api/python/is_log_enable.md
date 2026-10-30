# is\_log\_enable

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:02:51.893Z pushedAt=2026-10-29T03:15:27.440Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Queries whether logs of the corresponding level and type are enabled.

## Prototype

```python
is_log_enable(self, log_type: fw.FlowFuncLogType, log_level: fw.FlowFuncLogLevel) -> bool
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| log_type | Input | Log type. The value can be:<br><br>  - DEBUG_LOG<br>  - RUN_LOG |
| log_level | Input | Log level. The value can be:<br><br>  - DEBUG<br>  - INFO<br>  - WARN<br>  - ERROR |

## Return Value

- True: enabled
- False: disabled

## Exception Handling

None

## Constraints

None
