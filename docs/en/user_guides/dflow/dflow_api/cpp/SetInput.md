# SetInput

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T04:01:14.119Z pushedAt=2026-11-16T06:05:30.707Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the input of a FlowNode, indicating that the src\_op-th output of src\_index is used as the dst\_index-th input of the FlowNode, and returns the FlowNode node with the input set.

## Prototype

```cpp
FlowNode &SetInput(uint32_t dst_index, const FlowOperator &src_op, uint32_t src_index = 0)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dst_index | Input | Input index of the FlowNode. |
| src_op | Input | Node of the FlowNode input, which can only be FlowNode or FlowData. |
| src_index | Input | Output index of src_op. |

## Return Value

Returns the FlowNode node with the input set.

## Exception Handling

None.

## Constraints

None.
