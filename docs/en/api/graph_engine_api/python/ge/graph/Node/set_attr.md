# set\_attr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:12:27.526Z pushedAt=2026-10-16T10:53:07.465Z -->

## Applicable Products

Supported on all chips.

## Description

Sets node attributes.

## Prototype

```python
set_attr(key: str, value: Any) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| key | Input | Attribute name, of the string type. |
| value | Input | Attribute value. For the supported attribute value types, see [AttrValueType](../../AttrValueType.md). |

## Return Value

None

## Constraints

- If the parameter type is incorrect, **TypeError** is thrown.
- If the setting fails, **RuntimeError** is thrown.
