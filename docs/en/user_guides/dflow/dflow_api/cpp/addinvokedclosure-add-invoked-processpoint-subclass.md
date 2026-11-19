# AddInvokedClosure \(Adding the ProcessPoint Child Class to Be Called\)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:04:41.240Z pushedAt=2026-11-16T06:11:08.665Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Adds the GraphPp/FlowGraphPp called by FunctionPp and returns the added FunctionPp.

## Prototype

```cpp
FunctionPp &AddInvokedClosure(const char_t *name, const ProcessPoint &pp)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Unique identifier of the called GraphPp/FlowGraphPp, which must be unique across the entire graph. |
| pp | Input | Called GraphPp/FlowGraphPp |

## Return Value

Returns the configured FunctionPp.

## Exception Handling

None.

## Constraints

None.
