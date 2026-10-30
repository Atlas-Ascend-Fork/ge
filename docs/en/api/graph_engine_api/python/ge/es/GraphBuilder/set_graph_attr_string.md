# set\_graph\_attr\_string

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:51:10.622Z pushedAt=2026-10-16T10:53:07.382Z -->

## Applicable Products

Supported on all chips.

## Description

Sets a **string** attribute for the graph.

## Prototype

```python
set_graph_attr_string(attr_name: str, value: str) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **attr_name** | Input | Attribute name. |
| **value** | Input | String value. |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a **TypeError** is thrown.
- If the setting fails, a **RuntimeError** is thrown.
