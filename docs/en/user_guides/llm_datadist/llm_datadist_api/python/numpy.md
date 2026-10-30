# numpy

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:29:33.719Z pushedAt=2026-10-29T06:38:00.072Z nl=2 -->

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

Obtains the numpy data of a Tensor.

## Prototype

```python
numpy(copy=False)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| copy | bool | The value is False or True.<br>The default value is False, indicating that the data is converted from a Tensor to a numpy.ndarray without being copied. If the value is True, the data needs to be copied.<br>If the Tensor contains string data, set this parameter to True. Otherwise, an exception is thrown. |

## Examples

```python
tensor = Tensor(numpy.array([1]))
np_arr = tensor.numpy()
```

## Return Value

Returns a numpy.array.

## Constraints

None
