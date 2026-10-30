# FlowMsgQueue Constructor and Destructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T12:00:10.017Z pushedAt=2026-10-28T12:30:42.538Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Constructor and destructor of FlowMsgQueue.

## Prototype

```cpp
FlowMsgQueue() = default
~FlowMsgQueue() = default
```

## Parameters

None.

## Return Value

None.

## Exception Handling

None.

## Constraints

In streaming input scenarios (where the flow func function takes a queue as its input parameter), the framework does not support data alignment or proactive exception reporting by UDFs.
