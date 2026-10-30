# SetInputs

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:45:45.025Z pushedAt=2026-10-29T02:30:21.048Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the input nodes of a FlowGraph, automatically constructs a FlowGraph based on the output connection relationships of the nodes, and returns the graph.

## Prototype

```cpp
FlowGraph &SetInputs(const std::vector<FlowOperator> &inputs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| inputs | Input | Set of input nodes. |

## Return Value

Returns the FlowGraph with the input nodes built.

## Exception Handling

None.

## Constraints

None.
