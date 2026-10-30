# Warn-Level Log Macro of Debug Logs

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:48:42.585Z pushedAt=2026-10-29T01:12:23.950Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Debug log macro for Warn-level logs.

## Prototype

```cpp
FLOW_FUNC_LOG_WARN(fmt, ...)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| fmt | Input | Format control string of the log.<br>Value range: non-empty. |
| ... | Input | Optional parameters<br>Parameters that match fmt. |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
