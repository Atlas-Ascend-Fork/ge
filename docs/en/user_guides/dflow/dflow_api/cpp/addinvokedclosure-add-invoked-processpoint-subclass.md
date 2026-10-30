# AddInvokedClosure \(Adding the ProcessPoint Child Class to Be Called\)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:34:48.535Z pushedAt=2026-10-28T12:30:42.432Z nl=2 -->

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
