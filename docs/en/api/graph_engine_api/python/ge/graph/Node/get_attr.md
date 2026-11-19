# get\_attr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:06:44.036Z pushedAt=2026-10-16T10:53:07.442Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the node attribute.

## Prototype

```python
get_attr(key: str) -> Any
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **key** | Input | Attribute name, of the **string** type. |

## Return Value

Returns the attribute value.

## Constraints

- If **key** is not a string, a **TypeError** is thrown.
- If the retrieval fails, a **RuntimeError** is thrown.
