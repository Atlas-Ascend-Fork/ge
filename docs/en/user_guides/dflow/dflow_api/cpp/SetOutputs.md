# SetOutputs

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:49:32.471Z pushedAt=2026-10-29T02:33:12.868Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the output nodes of a FlowGraph and returns the graph.

## Prototype

```cpp
FlowGraph &SetOutputs(const std::vector<FlowOperator> &outputs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| outputs | Output | Set of output nodes. |

## Return Value

Returns the FlowGraph with the output nodes set.

## Exception Handling

None.

## Constraints

None.
