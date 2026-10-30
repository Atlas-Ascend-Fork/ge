# add\_input

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:05:40.581Z pushedAt=2026-10-16T10:53:07.666Z -->

## Applicable Products

Supported on all chips.

## Description

Binds the **index**-th boundary input to **SubgraphInput**.

## Prototype

```python
add_input(index: int, input: SubgraphInput) -> int
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Logical boundary slot index, which must be aligned with the input order of the replacement graph. |
| input | Input | Boundary input, of the type **SubgraphInput**. |

## Return Value

Returns the internal state.

## Constraints

None
