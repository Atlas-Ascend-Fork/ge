# add\_output

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:05:41.995Z pushedAt=2026-10-16T10:53:07.667Z -->

## Applicable Products

Supported on all chips.

## Description

Binds the **index**-th boundary output to **SubgraphOutput**.

## Prototype

```python
add_output(index: int, output: SubgraphOutput) -> int
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Logical boundary slot index, which must be aligned with the output order of the replacement graph. |
| output | Input | Boundary output, of the type **SubgraphOutput**. |

## Return Value

Returns the internal state.

## Constraints

None
