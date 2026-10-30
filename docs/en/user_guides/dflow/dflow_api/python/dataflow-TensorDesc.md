# dataflow.TensorDesc

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:15:43.613Z pushedAt=2026-10-29T03:04:34.913Z nl=2 -->

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

Returns None in normal scenarios.

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
tensor_desc = TensorDesc(DT_FLOAT, [1,2])
```

## Constraints

None
