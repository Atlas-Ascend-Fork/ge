# SetTransactionId (FlowMsg Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:52:48.172Z pushedAt=2026-10-29T02:37:15.474Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the transaction ID in a FlowMsg message. The transaction ID starts from 1 and is used to identify a batch of data. When it is set to 0, the custom transaction\_id is not used, and a transaction\_id is automatically generated in an auto-increment manner.

## Prototype

```cpp
virtual void SetTransactionId(uint64_t transaction_id)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transaction_id | Input | Transaction ID of the message |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
