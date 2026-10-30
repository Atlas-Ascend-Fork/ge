# set\_graph\_output

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:51:22.392Z pushedAt=2026-10-16T10:53:07.383Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the graph output.

## Prototype

```python
set_graph_output(tensor: TensorHolder, output_index: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **tensor** | Input | Tensor holder to be set as the output. |
| **output_index** | Input | Output index. |

## Return Value

None

## Constraints

If the parameter type is incorrect, a TypeError is thrown.
