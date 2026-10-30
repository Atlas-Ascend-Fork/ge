# get\_data\_size

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:51:19.460Z pushedAt=2026-10-29T03:11:35.626Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the data size of a Tensor.

## Prototype

```python
get_data_size(self) -> int
```

## Parameters

None

## Return Value

Returns the data size of the Tensor.

## Exception Handling

None

## Constraints

Modifying the returned TensorDesc information does not affect the existing TensorDesc information in the Tensor object.
