# get\_data\_type

<!-- md-trans-meta sourceCommit=5f639ce5c040cc9acb4c51426dd643c1cfc70c83 translatedAt=2026-11-17T02:39:13.219Z pushedAt=2026-11-18T02:41:08.852Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

Obtains the data type of the tensor.

## Prototype

```python
get_data_type() -> DataType
```

## Parameters

None

## Return Value

\(DataType\) Returns the tensor data type.

## Constraints

If the return value is not a valid **DataType**, a **ValueError** is raised.
