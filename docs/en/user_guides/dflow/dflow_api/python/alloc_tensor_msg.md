# alloc\_tensor\_msg

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:03:54.231Z pushedAt=2026-10-29T02:55:06.222Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Applies for a FlowMsg of the Tensor type based on the shape, data type, and alignment size.

## Prototype

```python
alloc_tensor_msg(self, shape: Union[List[int], Tuple[int]], dtype, align:Optional[int] = 64) -> FlowMsg
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape of the Tensor. |
| dtype | Input | Data type of the Tensor. |
| align | Input | Alignment size of the applied memory address. Value range: [32, 64, 128, 256, 512, 1024]. Default value: 64. |

## Return Value

Returns the FlowMsg instance.

## Exception Handling

If the Tensor pointer cannot be obtained, None is returned.

## Constraints

None
