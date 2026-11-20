# dataflow.TensorDesc

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:19:47.736Z pushedAt=2026-11-16T07:52:12.122Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Describes a Tensor.

## Prototype

```python
TensorDesc(dtype: DType, shape: Union[List[int], Tuple[int]])
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| dtype | DType | Describes the data type of the [Tensor](dataflow-Tensor.md). |
| shape | Union[List[int], Tuple[int]] | Describes the shape of the Tensor. Each element in the shape must be a value greater than or equal to 0. |

## Return Value

None is returned in normal scenarios.

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
tensor_desc = TensorDesc(DT_FLOAT, [1,2])
```

## Constraints

None
