# Info-Level Log Macro of Debug Logs

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:22:15.210Z pushedAt=2026-11-16T07:53:29.966Z nl=2 -->

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

```python
info(self, message: str, *args: tuple) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| message | Input | Log format control string.<br>Value range: non-empty. |
| args | Input | Optional parameter<br>Parameters that match message. |

## Return Value

None

## Exception Handling

None

## Constraints

None
