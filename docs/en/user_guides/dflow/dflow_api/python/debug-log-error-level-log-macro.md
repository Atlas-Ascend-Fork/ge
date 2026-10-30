# Error-Level Log Macro of Debug Logs

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:16:47.199Z pushedAt=2026-10-29T03:05:53.175Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Debug log macro for the Error level.

## Prototype

```python
error(self, message: str, *args: tuple) -> None
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
