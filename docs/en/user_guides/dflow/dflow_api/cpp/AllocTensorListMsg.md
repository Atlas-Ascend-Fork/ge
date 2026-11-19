# AllocTensorListMsg

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:09:03.907Z pushedAt=2026-11-16T06:11:08.681Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Allocates a contiguous buffer based on the input dtype shapes array to hold a Tensor array.

## Prototype

```cpp
virtual std::shared_ptr<FlowMsg> AllocTensorListMsg(const std::vector<std::vector<int64_t>> &shapes, const std::vector<TensorDataType> &dataTypes, uint32_t align = 512U)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shapes | Input | List of shapes corresponding to the Tensor List. |
| dataTypes | Input | List of dataTypes of the Tensor List. |
| align | Input | Alignment size of the applied memory address. Value range: [32, 64, 128, 256, 512, 1024]. |

## Return Value

Pointer to the applied FlowMsg.

## Exception Handling

If the Tensor pointer cannot be obtained, NULL is returned.

## Constraints

None.
