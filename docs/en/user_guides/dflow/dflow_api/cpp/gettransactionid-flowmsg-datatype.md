# GetTransactionId (FlowMsg Data Type)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:07:42.460Z pushedAt=2026-11-16T03:42:27.244Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the transaction ID in a FlowMsg message. The transaction ID starts from 1 and increments by 1 each time a batch of data is fed, which can be used to identify the batch of data.

## Prototype

```cpp
virtual uint64_t GetTransactionId() const
```

## Parameters

None.

## Return Value

transaction ID.

## Exception Handling

None.

## Constraints

None.
