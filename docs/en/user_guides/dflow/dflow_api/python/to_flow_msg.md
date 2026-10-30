# to\_flow\_msg

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T07:38:16.456Z pushedAt=2026-10-29T07:50:19.014Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Converts a dataflow Tensor into a FlowMsg.

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
