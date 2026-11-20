# dataflow.FlowOutput

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:15:25.180Z pushedAt=2026-11-16T07:51:11.578Z nl=2 -->

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
