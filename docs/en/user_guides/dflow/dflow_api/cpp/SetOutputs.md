# SetOutputs

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T04:05:32.648Z pushedAt=2026-11-16T06:07:10.480Z nl=2 -->

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
