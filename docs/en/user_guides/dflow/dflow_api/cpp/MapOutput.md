# MapOutput

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:19:33.364Z pushedAt=2026-11-16T04:53:49.817Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Maps the output of a FlowNode, indicating that the pp\_output\_index-th output of the ProcessPoint is passed to the node\_output\_index-th output of the FlowNode, and returns the FlowNode node after mapping.

This is an optional API. If it is not called, the outputs of the FlowNode and the ProcessPoint are mapped in sequence by default.

## Prototype

```cpp
FlowNode &MapOutput(uint32_t node_output_index, const ProcessPoint &pp, uint32_t pp_output_index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node_output_index | Input | Output index of the FlowNode. |
| pp | Input | ProcessPoint to which the FlowNode node is mapped. |
| pp_output_index | Input | Output index of the ProcessPoint. |

## Return Value

Returns the FlowNode node with the output mapped.

## Exception Handling

None.

## Constraints

Before calling this API, you must call [FlowNode &AddPp](AddPp.md).
