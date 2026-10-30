# get\_attr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:02:12.190Z pushedAt=2026-10-16T10:53:07.423Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains graph attributes.

## Prototype

```python
get_attr(key: str) -> Any
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **key** | Input | Attribute name, of the **string** type. |

## Return Value

Returns the attribute value.

## Constraints

- If **key** is not a string, a **TypeError** is thrown.
- If attribute retrieval fails, a **RuntimeError** is thrown.
