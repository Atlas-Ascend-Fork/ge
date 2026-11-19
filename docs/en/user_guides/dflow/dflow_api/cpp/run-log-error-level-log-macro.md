# Error-Level Log Macro of Run Logs

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:48:49.450Z pushedAt=2026-11-16T05:57:25.154Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Error-level log macro of run logs.

## Prototype

```cpp
FLOW_FUNC_RUN_LOG_ERROR(fmt, ...)
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
