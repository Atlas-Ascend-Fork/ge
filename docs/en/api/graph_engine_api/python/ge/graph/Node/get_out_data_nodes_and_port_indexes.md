# get\_out\_data\_nodes\_and\_port\_indexes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:09:20.747Z pushedAt=2026-10-16T10:53:07.453Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the list of data nodes and port indexes for the specified **out_index**.

## Prototype

```python
get_out_data_nodes_and_port_indexes(out_index: int) -> List[Tuple[Node, int]]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **out_index** | Input | Output index. |

## Return Value

\(List\[Tuple\[Node, int\]\]\) \( Returns a list of output nodes and port indexes.\)

## Constraints

- If out\_index is not an integer, **TypeError** is thrown.
- If the retrieval fails, **RuntimeError** is thrown.
