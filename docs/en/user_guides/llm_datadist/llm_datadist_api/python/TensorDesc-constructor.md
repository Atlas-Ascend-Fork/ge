# TensorDesc-constructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:36:58.508Z pushedAt=2026-10-29T06:46:49.493Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 inference products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 training products: Not supported
<!-- end id3 -->

## Function

Constructs a TensorDesc.

## Prototype

```python
__init__(dtype: DataType, shape: Union[List[int], Tuple[int]])
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| dtype | [DataType](DataType.md) | Indicates the data type of the tensor. |
| shape | Union[List[int], Tuple[int]] | Indicates the description of the tensor shape. |

## Return Value

Returns a TensorDesc instance under normal conditions.

If the input data type is incorrect, a TypeError or ValueError exception is thrown.

## Examples

```python
from llm_datadist import TensorDesc, DataType
tensor_desc = TensorDesc(DataType.DT_FLOAT, [1,2])
```

## Constraints

None
