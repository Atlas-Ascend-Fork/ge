# numpy

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:08:50.829Z pushedAt=2026-11-17T03:39:12.006Z nl=2 -->

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

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
tensor = Tensor(1)
array = tensor.numpy()
```

## Constraints

None
