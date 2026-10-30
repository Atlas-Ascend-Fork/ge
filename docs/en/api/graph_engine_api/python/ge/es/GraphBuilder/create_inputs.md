# create\_inputs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:45:19.917Z pushedAt=2026-10-16T10:53:07.363Z -->

## Applicable Products

Supported on all chips.

## Description

Creates multiple inputs.

## Prototype

```python
create_inputs(num: int, start_index: int = 0) -> List[TensorHolder]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| num | Input | Number of inputs to create. |
| start_index | Input | Starting index of the inputs. A non-zero value indicates that other inputs have already been created. |

## Return Value

\(List\[TensorHolder\]\) Returns a list of input tensor holders.

## Constraints

- Input node indexes must start from 0 and increase consecutively.
- If the parameter type is incorrect, **TypeError** is thrown.
- If input creation fails, **RuntimeError** is thrown.
