# get\_in\_data\_nodes\_and\_port\_indexes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:07:29.271Z pushedAt=2026-10-16T10:53:07.445Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the data node and port index of the specified input index.

## Prototype

```python
get_in_data_nodes_and_port_indexes(in_index: int) -> Tuple[Node, int]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| in_index | Input | Input index. |

## Return Value

\(Tuple\[Node, int\]\) \(Input node and port index \).

## Constraints

- If **in\_index** is not an integer, a TypeError is thrown.
- If the acquisition fails, a RuntimeError is thrown.
