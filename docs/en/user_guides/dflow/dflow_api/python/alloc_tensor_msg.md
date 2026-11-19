# alloc\_tensor\_msg

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:07:15.720Z pushedAt=2026-11-16T07:44:10.810Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Requests a FlowMsg of the Tensor type based on the shape, data type, and alignment size.

## Prototype

```python
alloc_tensor_msg(self, shape: Union[List[int], Tuple[int]], dtype, align:Optional[int] = 64) -> FlowMsg
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape of the Tensor. |
| dtype | Input | Data type of the Tensor. |
| align | Input | Alignment size of the applied memory address. Value range: [32, 64, 128, 256, 512, 1024]. The default value is 64. |

## Return Value

Returns the FlowMsg instance.

## Exception Handling

If the Tensor pointer cannot be obtained, None is returned.

## Constraints

None
