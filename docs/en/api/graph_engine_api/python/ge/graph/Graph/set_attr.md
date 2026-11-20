# set\_attr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:06:05.570Z pushedAt=2026-10-16T10:53:07.440Z -->

## Applicable Products

Supported on all chips.

## Description

Sets graph attributes.

## Prototype

```python
set_attr(key: str, value: Any) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **key** | Input | Attribute name, of the string type. |
| **value** | Input | Attribute value. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a TypeError is thrown.
- If the attribute fails to be set, a RuntimeError is thrown.
