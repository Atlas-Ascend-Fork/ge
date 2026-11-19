# set\_init\_param

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:31:49.146Z pushedAt=2026-11-17T03:39:12.041Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the initialization parameters of FuncProcessPoint.

## Prototype

```python
set_init_param(attr_name, attr_value)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| attr_name | str | Name of the initialization parameter. |
| attr_value | Union[str,List[str],int,List[int],List[List[int]],float,List[float],bool,List[bool],DType,List[DType]] | Value of the initialization parameter. |

## Return Value

None is returned in normal scenarios.

Returns "TypeError" if the parameter type is incorrect.

## Examples

```python
import dataflow as df
pp = df.FuncProcessPoint(...)
pp0.set_init_param("out_type", df.DT_INT32) # Set based on the actual UDF implementation.
```

## Constraints

None
