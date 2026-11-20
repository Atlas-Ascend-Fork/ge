# find\_node\_by\_name

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:01:01.986Z pushedAt=2026-10-16T10:53:07.419Z -->

## Applicable Products

Supported on all chips.

## Description

Finds a node by name.

## Prototype

```python
find_node_by_name(name: str) -> Node
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Node name. |

## Return Value

Returns the found node.

## Constraints

- If **name** is not a string, a TypeError is thrown.
- If the lookup fails, a RuntimeError is thrown.
