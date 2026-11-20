# AllocTensor (FlowBufferFactory Data Type)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:07:55.828Z pushedAt=2026-11-16T06:11:08.678Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Allocates a Tensor based on the shape, data type, and alignment size.

## Prototype

```cpp
std::shared_ptr<Tensor> AllocTensor(const std::vector<int64_t> &shape, DataType data_type, uint32_t align = 512U)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape of the Tensor. |
| data_type | Input | DataType of the Tensor. |
| align | Input | Alignment size of the applied memory address. The value range is [32, 64, 128, 256, 512, 1024]. This parameter is currently reserved. |

## Return Value

Pointer to the allocated Tensor.

## Exception Handling

If the Tensor pointer cannot be obtained, NULL is returned.

## Constraints

None.
