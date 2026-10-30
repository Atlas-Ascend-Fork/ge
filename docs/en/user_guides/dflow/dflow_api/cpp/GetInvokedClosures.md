# GetInvokedClosures

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:36:36.847Z pushedAt=2026-10-29T01:41:36.208Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the GraphPp invoked by FunctionPp.

## Prototype

```cpp
const std::map<const std::string, const GraphPp> &GetInvokedClosures() const
```

## Parameters

None

## Return Value

Returns the map of <name, graph\_pp\>** set by [AddInvokedClosure](addinvokedclosure-add-invoked-graph-pp.md).

## Exception Handling

None.

## Constraints

None.
