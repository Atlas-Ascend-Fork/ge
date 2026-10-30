# AllocTensor (FlowBufferFactory Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:38:10.312Z pushedAt=2026-10-28T12:30:42.442Z nl=2 -->

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
| align | Input | Alignment size of the allocated memory address. The value range is [32, 64, 128, 256, 512, 1024]. |

## Return Value

Pointer to the allocated Tensor.

## Exception Handling

If the Tensor pointer cannot be obtained, NULL is returned.

## Constraints

None.
