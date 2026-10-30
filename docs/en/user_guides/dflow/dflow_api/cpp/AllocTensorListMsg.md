# AllocTensorListMsg

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:39:11.420Z pushedAt=2026-10-28T12:30:42.445Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Allocates a contiguous memory block based on the input dtype shapes array to hold a Tensor array.

## Prototype

```cpp
virtual std::shared_ptr<FlowMsg> AllocTensorListMsg(const std::vector<std::vector<int64_t>> &shapes, const std::vector<TensorDataType> &dataTypes, uint32_t align = 512U)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shapes | Input | List of shapes corresponding to the Tensor List. |
| dataTypes | Input | List of dataTypes of the Tensor List. |
| align | Input | Alignment size of the requested memory address. Value range: [32, 64, 128, 256, 512, 1024]. |

## Return Value

Pointer to the applied FlowMsg.

## Exception Handling

NULL is returned if the Tensor pointer cannot be allocated.

## Constraints

None.
