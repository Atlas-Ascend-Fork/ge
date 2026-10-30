# MapOutput

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:58:25.296Z pushedAt=2026-10-29T01:54:58.522Z nl=2 -->

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
