# to\_flow\_msg

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:40:25.362Z pushedAt=2026-11-17T03:39:12.063Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Convert a dataflow Tensor into a FlowMsg.

## Prototype

```python
to_flow_msg(self, tensor)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Dataflow Tensor to be converted. |

## Return Value

Returns a FlowMsg instance on success. Returns None on failure.

## Exception Handling

None

## Constraints

None
