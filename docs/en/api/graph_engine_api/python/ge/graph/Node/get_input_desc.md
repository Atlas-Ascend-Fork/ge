# get\_input\_desc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:08:07.857Z pushedAt=2026-10-16T10:53:07.448Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the Tensor format of the specified input port.

## Prototype

```python
get_input_desc(index: int) -> TensorDesc
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **index** | Input | Input index. |

## Return Value

Returns the obtained Tensor format.

## Constraints

- If **index** is not an integer, a **TypeError** is thrown.
- If the Tensor format fails to be obtained, a **RuntimeError** is thrown.
