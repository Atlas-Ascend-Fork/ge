# get\_data\_type

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:24:49.250Z pushedAt=2026-10-16T10:53:07.520Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the data type of the **Tensor** described by **TensorDesc**.

## Prototype

```python
get_data_type() -> DataType
```

## Parameters

None

## Return Value

Returns a **DataType** enumeration value.

## Constraints

Because the returned **DataType** information is a value copy, modifying the returned **DataType** information does not affect the existing **DataType** information in **TensorDesc**.
