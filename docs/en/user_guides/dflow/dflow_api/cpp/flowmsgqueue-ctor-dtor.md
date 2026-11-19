# FlowMsgQueue Constructor and Destructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:29:33.182Z pushedAt=2026-11-16T06:11:08.768Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

FlowMsgQueue constructor and destructor.

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
