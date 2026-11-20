# SetContainsNMappingNode

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:54:54.303Z pushedAt=2026-11-16T06:02:51.468Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets whether the FlowGraph contains an n\_mapping node. An n\_mapping node refers to a node that meets any of the following conditions.

- One input produces non-single (multiple, zero, or variable) outputs.
- Multiple inputs produce a single output.
- Multiple inputs produce non-single (multiple, zero, or variable) outputs.

## Prototype

```cpp
FlowGraph &SetContainsNMappingNode(bool contains_n_mapping_node)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| contains_n_mapping_node | Input | Whether the graph contains an n_mapping node.<br>If the graph contains at least one n_mapping processing node, set this parameter to true. The default value is false. |

## Return Value

Returns the FlowGraph with the property of whether it contains an n\_mapping node set.

## Exception Handling

None.

## Constraints

None.
