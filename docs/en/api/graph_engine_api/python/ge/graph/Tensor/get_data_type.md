# get\_data\_type

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:20:40.287Z pushedAt=2026-10-16T10:53:07.501Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the data type of the tensor.

## Prototype

```python
get_data_type() -> DataType
```

## Parameters

None

## Return Value

\(DataType\) Returns the tensor data type.

## Constraints

If the return value is not a valid **DataType**, a **ValueError** is raised.
