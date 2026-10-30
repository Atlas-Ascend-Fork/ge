# Dequeue

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:50:24.461Z pushedAt=2026-10-29T01:12:46.225Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the balanced distribution affinity.

## Prototype

```cpp
virtual int32_t Dequeue(std::shared_ptr<FlowMsg> &flowMsg, int32_t timeout = -1) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| flowMsg | Output | FlowMsg dequeued. |
| timeout | Input | Dequeue timeout in ms. The default value is -1, which means the current thread keeps blocking until an element is available in the queue. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
