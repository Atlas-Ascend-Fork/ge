# set\_transaction\_id

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:17:28.824Z pushedAt=2026-10-29T03:23:03.027Z nl=2 -->

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

```python
set_transaction_id(self, transaction_id) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transaction_id | Input | Transaction ID used for data transfer.<br>When set to 0, it indicates that no custom transaction_id is used, and a transaction_id is automatically generated internally in an incrementing manner.<br>In data alignment scenarios, the transaction_id is used for routing and alignment. |

## Return Value

None

## Exception Handling

None

## Constraints

None
