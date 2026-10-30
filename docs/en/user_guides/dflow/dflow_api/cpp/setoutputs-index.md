# SetOutputs (Index)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:48:45.377Z pushedAt=2026-10-29T02:33:31.252Z nl=2 -->

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
