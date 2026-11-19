# SetOutputs (index)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T04:05:20.682Z pushedAt=2026-11-16T06:07:03.158Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the association between a FlowNode in a FlowGraph and the output index of the FlowNode, and returns the graph. It is commonly used to set partial outputs of a FlowNode. For example, if FlowNode1 has two outputs but only one output of FlowNode1 is required as the input of FlowNode2, an output index of FlowNode1 can be set in this case.

## Prototype

```cpp
FlowGraph &SetOutputs(const std::vector<std::pair<FlowOperator, std::vector<size_t>>> &output_indexes)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| output_indexes | Input | Set of associations between output nodes and node output indexes. |

## Return Value

Returns the FlowGraph with the associations between output nodes and node output indexes set.

## Exception Handling

None.

## Constraints

None.
