# set\_transaction\_id

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:36:30.967Z pushedAt=2026-11-17T03:39:12.052Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the transaction ID used for DataFlow data transfer.

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
