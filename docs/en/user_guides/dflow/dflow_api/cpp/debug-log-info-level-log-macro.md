# Info-Level Log Macro of Debug Logs

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:48:42.248Z pushedAt=2026-10-29T01:11:53.010Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Debug log macro at the Info level.

## Prototype

```cpp
FLOW_FUNC_LOG_INFO(fmt, ...)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| fmt | Input | Log format control string.<br>Value range: non-empty. |
| ... | Input | Optional parameters<br>Parameters that match fmt. |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
