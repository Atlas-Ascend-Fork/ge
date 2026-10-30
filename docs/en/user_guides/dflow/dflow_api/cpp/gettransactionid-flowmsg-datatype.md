# GetTransactionId (FlowMsg Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:47:53.096Z pushedAt=2026-10-29T01:48:04.094Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the transaction ID from a FlowMsg message. The transaction ID starts from 1 and increments by 1 each time a batch of data is fed, which can be used to identify which batch of data it is.

## Prototype

```cpp
virtual uint64_t GetTransactionId() const
```

## Parameters

None.

## Return Value

Transaction ID.

## Exception Handling

None.

## Constraints

None.
