# AllocRawDataMsg (MetaRunContext Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:37:27.484Z pushedAt=2026-10-28T12:30:42.440Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Applies for a contiguous memory block of the specified size to store RawData type data.

## Prototype

```cpp
virtual std::shared_ptr<FlowMsg> AllocRawDataMsg(int64_t size, uint32_t align = 512U)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| size | Input | Size of the memory to be applied for. |
| align | Input | Alignment size of the memory address to be applied for. The value range is [32, 64, 128, 256, 512, 1024]. |

## Return Value

Pointer to the requested FlowMsg.

## Exception Handling

If the Tensor pointer cannot be obtained, NULL is returned.

## Constraints

None.
