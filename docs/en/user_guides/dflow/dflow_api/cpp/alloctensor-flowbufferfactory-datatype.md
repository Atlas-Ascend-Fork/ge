# AllocTensor (FlowBufferFactory Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:38:40.797Z pushedAt=2026-10-28T12:30:42.443Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Applies for a Tensor based on the shape, data type, and alignment size.

## Prototype

```cpp
std::shared_ptr<Tensor> AllocTensor(const std::vector<int64_t> &shape, DataType data_type, uint32_t align = 512U)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape of the Tensor. |
| data_type | Input | DataType of the Tensor. |
| align | Input | Alignment size of the applied memory address. The value ranges from 32, 64, 128, 256, 512, to 1024. This parameter is currently reserved. |

## Return Value

Pointer to the allocated Tensor.

## Exception Handling

If the Tensor pointer cannot be allocated, NULL is returned.

## Constraints

None.
