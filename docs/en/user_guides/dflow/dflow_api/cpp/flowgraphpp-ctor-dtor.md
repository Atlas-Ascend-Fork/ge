# FlowGraphPp Constructor and Destructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:57:59.833Z pushedAt=2026-10-28T12:30:42.527Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

The FlowGraphPp constructor and destructor. The constructor returns a FlowGraphPp object.

## Prototype

```cpp
FlowGraphPp(const char_t *pp_name, const FlowGraphBuilder &builder)
~FlowGraphPp() override
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pp_name | Input | Name of the FlowGraphPp, which must be unique across the entire graph. |
| builder | Input | Method for building the FlowGraph: std::function<FlowGraph()><br>For details about how to build the FlowGraph, see [FlowGraph Class](flowgraph-class.md). |

## Return Value

Returns a FlowGraphPp object.

## Exception Handling

None.

## Constraints

None.
