# data\_type

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:10:31.670Z pushedAt=2026-10-16T10:53:07.692Z -->

## Applicable Products

Supported on all chips

## Description

Gets the data type of **Tensor**.

## Prototype

```python
tensor.data_type -> DataType
```

## Parameters

None

## Return Value

Returns the **Tensor** `DataType`.

## Constraints

The return value is a copy of the **DataType** enumeration value.

## Example

```python
data_type = x.data_type
```
