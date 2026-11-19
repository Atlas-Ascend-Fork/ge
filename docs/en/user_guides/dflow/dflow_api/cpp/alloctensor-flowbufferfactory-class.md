# AllocTensor (FlowBufferFactory Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:07:41.788Z pushedAt=2026-11-16T06:11:08.676Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

To reduce the time consumed by input and output copying, the FlowBufferFactory class is provided for constructing types that use shared memory.

AllocTensor allocates a Tensor based on the shape, data type, and alignment size. By default, the memory is aligned to 64 bytes. You can specify the alignment size to facilitate performance tuning.

## Prototype

```cpp
std::shared_ptr<Tensor> AllocTensor(const std::vector<int64_t> &shape, TensorDataType dataType, uint32_t align = 512U)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Shape of the Tensor. |
| dataType | Input | Data type of the Tensor. |
| align | Input | Alignment size of the applied memory address. Value range: [32, 64, 128, 256, 512, 1024]. |

## Return Value

Pointer to the allocated Tensor.

## Exception Handling

If the Tensor pointer cannot be obtained, NULL is returned.

## Constraints

None.
