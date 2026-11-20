# GetInvokedClosures

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T02:55:02.871Z pushedAt=2026-11-16T03:39:16.257Z nl=2 -->

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
