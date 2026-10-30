# set\_init\_param

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:14:14.837Z pushedAt=2026-10-29T03:22:12.242Z nl=2 -->

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

Returns None in normal scenarios.

Returns "TypeError" if the parameter type is incorrect.

## Examples

```python
import dataflow as df
pp = df.FuncProcessPoint(...)
pp0.set_init_param("out_type", df.DT_INT32) # Set based on the actual UDF implementation
```

## Constraints

None
