# has\_attr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:10:56.841Z pushedAt=2026-10-16T10:53:07.459Z -->

## Applicable Products

Supported on all chips.

## Description

Checks whether the node has the specified attribute.

## Prototype

```python
has_attr(attr_name: str) -> bool
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_name | Input | Attribute name, of the **string** type. |

## Return Value

\(bool\) Returns whether the attribute exists.

## Constraints

If attr\_name is not a string, a **TypeError** is thrown.
