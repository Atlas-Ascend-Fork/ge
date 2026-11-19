# Dequeue

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:19:25.769Z pushedAt=2026-11-16T06:11:08.724Z nl=2 -->

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

- 0: SUCCESS
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
