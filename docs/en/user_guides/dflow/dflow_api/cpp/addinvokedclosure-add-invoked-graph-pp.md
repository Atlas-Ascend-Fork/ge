# AddInvokedClosure \(Adding GraphPp to Be Called\)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:04:18.228Z pushedAt=2026-11-16T06:11:08.663Z nl=2 -->

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
