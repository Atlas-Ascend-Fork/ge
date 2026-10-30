# AllocTensorMsgWithAlign (MetaRunContext Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:41:17.129Z pushedAt=2026-10-28T12:30:42.453Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Applies for a FlowMsg of the Tensor type based on the shape, data type, and alignment size. The difference from the [AllocTensorMsg](alloctensormsg-metaruncontext-class.md) function is that AllocTensorMsg applies for memory aligned to 64 bytes by default, whereas this function allows you to specify the alignment size for performance tuning.

## Prototype

```cpp
std::shared_ptr<FlowMsg> AllocTensorMsgWithAlign(const std::vector<int64_t> &shape, TensorDataType dataType, uint32_t align)
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
