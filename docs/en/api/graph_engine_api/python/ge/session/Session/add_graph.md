# add\_graph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:16:40.592Z pushedAt=2026-10-16T10:53:07.718Z -->

## Description

Adds a graph to the session.

## Prototype

```python
add_graph(graph_id: int, add_graph: Graph, options: dict = None) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **graph_id** | Input | Graph ID. |
| **add_graph** | Input | Graph object to be added. |
| **options** | Input | Configuration options. For supported configuration items, see the graph-level parameters in [Options Parameter Description](../../../../cpp/ge/options_params/options_parameters_description.md). |

## Return Value

None

## Constraints

- If **graph\_id** is not an integer, a TypeError is thrown.
- If **add\_graph** is not of the **Graph** type, a TypeError is thrown.
- If **options** is not of the dictionary type, a TypeError is thrown.
- If the addition fails, a RuntimeError is thrown.
- If the same **Graph** object calls this API for registration, different **Graph ID**s actually share the same **Graph** object, causing subsequent operations to affect each other and result in errors.
- Do not use the same **Graph ID** to add different **Graph** objects. In this case, only the first added **Graph** object is retained, and subsequent **Graph** objects are not added successfully.
