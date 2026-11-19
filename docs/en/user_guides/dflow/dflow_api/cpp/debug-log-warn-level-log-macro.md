# Warn-Level Log Macro of Debug Logs

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:17:43.247Z pushedAt=2026-11-16T06:11:08.719Z nl=2 -->

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
| fmt | Input | Log format control string.<br>Value range: non-empty. |
| ... | Input | Optional parameter<br>Parameters matching fmt. |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
