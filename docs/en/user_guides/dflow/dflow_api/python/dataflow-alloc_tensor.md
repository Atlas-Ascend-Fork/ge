# dataflow.alloc\_tensor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:09:27.500Z pushedAt=2026-11-16T07:45:55.938Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Requests a dataflow tensor based on the shape, data type, and alignment size.

## Prototype

```python
alloc_tensor(shape: Union[List[int], Tuple[int]], dtype, align:Optional[int] = 64) -> Tensor
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape of the Tensor. |
| dtype | Input | Data type of the Tensor. |
| align | Input | Alignment size of the requested memory address. Value range: [32, 64, 128, 256, 512, 1024]. The default value is 64. |

## Return Value

Returns the Tensor instance.

## Exception Handling

If the tensor pointer cannot be obtained, None is returned.

## Constraints

None
