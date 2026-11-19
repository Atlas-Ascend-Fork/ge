# create\_input

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:45:09.199Z pushedAt=2026-10-16T10:53:07.362Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a graph input.

## Prototype

```python
create_input(index: int, *, name: Optional[str] = None, type_str: Optional[InputType] = InputType.DATA, data_type: Optional[DataType] = DataType.DT_FLOAT, format: Optional[Format] = Format.FORMAT_ND, shape: Optional[List[int]] = None) -> TensorHolder
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **index** | Input | Input index. |
| **name** | Input | Optional input. Input name, defaulting to "input_{index}". |
| **type_str** | Input | Optional input. Input type, defaulting to **DATA**. See [InputType](../../es/InputType.md). |
| **data_type** | Input | Optional input. Data type, defaulting to **DT_FLOAT**. See [DataType](../../DataType.md). |
| **format** | Input | Optional input. Data format, defaulting to **FORMAT_ND**. See [Format](../../Format.md). |
| **shape** | Input | Optional input. Shape dimension list, where **None** indicates a scalar. |

## Return Value

\(TensorHolder\) Returns the tensor holder of the input.

## Constraints

- If the parameter type is incorrect, a **TypeError** is thrown.
- If input creation fails, a **RuntimeError** is thrown.
