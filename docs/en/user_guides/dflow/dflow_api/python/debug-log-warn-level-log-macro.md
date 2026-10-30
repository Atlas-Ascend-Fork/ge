# Warn-Level Log Macro of Debug Logs

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:17:32.954Z pushedAt=2026-10-29T03:06:16.996Z nl=2 -->

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

```python
warn(self, message: str, *args: tuple) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| message | Input | Log format control string.<br>Value range: non-empty. |
| args | Input | Optional parameter.<br>Parameter that matches message. |

## Return Value

None

## Exception Handling

None

## Constraints

None
