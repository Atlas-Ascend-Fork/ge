# AddInvokedClosure \(Adding GraphPp to Be Called\)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:33:58.658Z pushedAt=2026-10-29T01:30:12.412Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Adds the GraphPp invoked by FunctionPp and returns the updated FunctionPp.

## Prototype

```cpp
FunctionPp &AddInvokedClosure(const char_t *name, const GraphPp &graph_pp)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Unique identifier of the invoked GraphPp, which must be unique across the entire graph. |
| graph_pp | Input | Invoked GraphPp. |

## Return Value

Returns the configured FunctionPp.

## Exception Handling

None.

## Constraints

None.
