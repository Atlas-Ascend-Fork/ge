# SetInputs

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T04:01:30.268Z pushedAt=2026-11-16T06:05:37.647Z nl=2 -->

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
