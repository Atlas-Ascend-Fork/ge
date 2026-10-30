# SetTransactionId (DataFlowInfo Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:52:03.611Z pushedAt=2026-10-29T02:35:25.485Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the transaction ID used for DataFlow data transmission.

## Prototype

```cpp
void SetTransactionId(uint64_t transaction_id)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transaction_id | Input | Transaction ID used for data transfer.<br>When set to 0, no custom transaction_id is used, and a transaction_id is automatically generated internally in an incrementing manner.<br>In data alignment scenarios, transaction_id is used for routing and alignment. |

## Return Value

None.

## Exception Handling

None.

## Constraints

- It takes effect only when the graph construction API sets SetContainsNMappingNode to true.
- transaction\_id can only increase, not decrease. If it is not set externally, transaction\_id auto-increments starting from 1.
- An error is reported after transaction\_id reaches the uint64\_max value.
- When data alignment is enabled, ensure that the transaction\_id of each batch of input data is consistent; otherwise, data may be misaligned.
- Custom transaction\_id is enabled only when a non-zero value is passed to the SetTransactionId API.
