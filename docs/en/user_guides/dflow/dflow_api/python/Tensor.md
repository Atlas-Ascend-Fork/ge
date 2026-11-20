# Tensor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:40:09.203Z pushedAt=2026-11-17T03:39:12.061Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Constructor of Tensor.

## Prototype

```python
Tensor(data, *, tensor_desc: TensorDesc=None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| data | Any | Data of the tensor, which can be a Python list or numpy. |
| tensor_desc | TensorDesc | Describes the tensor. When specified, if the dtype of the input data is inconsistent with the dtype of tensor_desc, the data is forcibly converted to the dtype of tensor_desc. If the shape of the input data is inconsistent with the shape of tensor_desc, an error is reported. |

## Return Value

None is returned in normal scenarios.

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
tensor = Tensor([1])
tensor = Tensor(numpy.array([1]))
tensor = Tensor([1], tensor_desc=TensorDesc(dataflow.DT_FLOAT, [1]))
```

## Constraints

None
