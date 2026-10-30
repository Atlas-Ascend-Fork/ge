# get\_output\_desc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:09:57.956Z pushedAt=2026-10-16T10:53:07.456Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the Tensor format of the specified output port.

## Prototype

```python
get_output_desc(index: int) -> TensorDesc
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **index** | Input | Output port number of the operator. |

## Return Value

Returns the obtained **Tensor format**.

## Constraints

- If **index** is not an integer, a **TypeError** is raised.
- If the Tensor format fails to be obtained, a **RuntimeError** is raised.
