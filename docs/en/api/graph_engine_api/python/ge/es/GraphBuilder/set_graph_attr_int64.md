# set\_graph\_attr\_int64

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:50:50.050Z pushedAt=2026-10-16T10:53:07.381Z -->

## Applicable Products

Supported on all chips.

## Description

Sets an int64 attribute for the graph.

## Prototype

```python
set_graph_attr_int64(attr_name: str, value: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **attr_name** | Input | Attribute name. |
| **value** | Input | Integer value. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, **TypeError** is thrown.
- If the setting fails, **RuntimeError** is thrown.
