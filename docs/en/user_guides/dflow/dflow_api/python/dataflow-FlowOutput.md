# dataflow.FlowOutput

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:12:35.313Z pushedAt=2026-10-29T03:03:25.844Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Describes the output of a FlowNode.

Users do not need to define it themselves. Calling the \_\_call\_\_ method of FlowNode returns FlowOutput or Tuple\[FlowOutput\].

## Prototype

```python
FlowOutput(node, index)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| node | FlowNode | FlowNode node. For details, see [dataflow.FlowNode](dataflow-FlowNode.md). |
| index | int | Output index of the FlowNode node |

## Return Value

None is returned in normal scenarios.

## Examples

None

## Constraints

None
