# SetTransactionId (FlowMsg Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:52:45.164Z pushedAt=2026-10-29T02:37:11.106Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the transaction ID in a FlowMsg message. The transaction ID starts from 1 and can be used to identify which batch of data it belongs to. When set to 0, it indicates that the custom transaction\_id is not used, and the transaction\_id is automatically generated in an incrementing manner internally.

## Prototype

```cpp
virtual void SetTransactionId(uint64_t transactionId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transactionId | Input | Transaction ID of the message |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
