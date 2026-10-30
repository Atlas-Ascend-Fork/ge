# AddInvokedClosure \(Adding FlowGraphPp to Be Called\)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:34:01.847Z pushedAt=2026-10-29T01:30:10.499Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: supported
<!-- end id2 -->

## Function

Adds a FlowGraphPp that invokes a FunctionPp, and returns the added FunctionPp.

## Prototype

```cpp
FunctionPp &AddInvokedClosure(const char_t *name, const FlowGraphPp &graph_pp)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Unique identifier of the invoked FlowGraphPp, which must be unique across the entire graph. |
| graph_pp | Input | FlowGraphPp to be invoked. |

## Return Value

Returns the configured FunctionPp.

## Exception Handling

None.

## Constraints

None.
