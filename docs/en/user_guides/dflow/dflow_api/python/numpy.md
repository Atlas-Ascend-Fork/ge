# numpy

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:05:40.433Z pushedAt=2026-10-29T03:17:54.885Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Converts a Tensor to a numpy ndarray.

## Prototype

```python
numpy(copy=False)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| copy | bool | The default value is False, indicating that the data is converted from a Tensor to a numpy.ndarray without being copied. If the value is True, the data needs to be copied.<br>If the Tensor contains string data, set this parameter to True. Otherwise, an exception is thrown. |

## Return Value

Returns a numpy.array.

In exceptional cases, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
tensor = Tensor(1)
array = tensor.numpy()
```

## Constraints

None
