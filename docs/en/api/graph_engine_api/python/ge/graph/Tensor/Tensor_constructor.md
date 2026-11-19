# Tensor Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:23:54.529Z pushedAt=2026-10-16T10:53:07.517Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a **Tensor** object.

## Prototype

```python
Tensor(data: Optional[UnionTensorDataType] = None, file_path: Optional[str] = None, data_type: Optional[DataType] = DataType.DT_FLOAT, format: Optional[Format] = Format.FORMAT_ND, shape: Optional[List[int]] = None, placement: Optional[Placement] = Placement.PLACEMENT_HOST)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data | Input | Data list. |
| file_path | Input | Path from which data is read. |
| data_type | Input | Data type, defaulting to DT_FLOAT. |
| format | Input | Data format, defaulting to FORMAT_ND. |
| shape | Input | List of shape dimensions. None indicates a scalar. |
| placement | Input | Storage location of the Tensor data, defaulting to PLACEMENT_HOST. |

## Return Value

None

## Constraints

- **data** and file\_path must be provided exclusively.
- If both are provided, a **RuntimeError** is raised.
- **shape** must be a list of integers.
- The DT\_DOUBLE type is not supported in the Python **Tensor** constructor.

## Examples

```python
# Create from data.
tensor1 = Tensor(data=[1.0, 2.0, 3.0], data_type=DataType.DT_FLOAT, format=Format.FORMAT_ND, shape=[3])

# Create from file.
tensor2 = Tensor(file_path="/path/to/file", data_type=DataType.DT_FLOAT, format=Format.FORMAT_ND, shape=[2, 2])
```
