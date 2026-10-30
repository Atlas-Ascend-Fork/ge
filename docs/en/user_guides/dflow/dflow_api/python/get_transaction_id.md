# get\_transaction\_id

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:00:22.500Z pushedAt=2026-10-29T03:13:56.850Z nl=2 -->

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

```python
get_transaction_id(self) -> int
```

## Parameters

None

## Return Value

transaction ID.

## Exception Handling

None

## Constraints

None
