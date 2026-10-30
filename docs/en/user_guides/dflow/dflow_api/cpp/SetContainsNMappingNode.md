# SetContainsNMappingNode

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:39:24.514Z pushedAt=2026-10-29T02:14:25.275Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets whether the FlowGraph contains an n\_mapping node. An n\_mapping node refers to any of the following cases.

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
