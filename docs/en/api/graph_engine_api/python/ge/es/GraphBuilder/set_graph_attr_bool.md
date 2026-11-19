# set\_graph\_attr\_bool

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:50:20.449Z pushedAt=2026-10-16T10:53:07.379Z -->

## Applicable Products

Supported on all chips.

## Description

Sets a bool attribute for the graph.

## Prototype

```python
set_graph_attr_bool(attr_name: str, value: bool) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **attr_name** | Input | Attribute name. |
| **value** | Input | Boolean value. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a TypeError is thrown.
- If the setting fails, a RuntimeError is thrown.
