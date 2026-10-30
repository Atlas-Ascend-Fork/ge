# dataflow.alloc\_tensor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:05:52.640Z pushedAt=2026-10-29T02:56:37.593Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Applies for a dataflow tensor based on the shape, data type, and alignment size.

## Prototype

```python
alloc_tensor(shape: Union[List[int], Tuple[int]], dtype, align:Optional[int] = 64) -> Tensor
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape of the Tensor. |
| dtype | Input | Data type of the Tensor. |
| align | Input | Alignment size of the applied memory address. Value range: [32, 64, 128, 256, 512, 1024]. The default value is 64. |

## Return Value

Returns the Tensor instance.

## Exception Handling

If the tensor pointer cannot be obtained, None is returned.

## Constraints

None
