# FlowNode Constructor and Destructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T12:00:58.176Z pushedAt=2026-10-28T12:30:42.543Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function Description

FlowNode constructor and destructor. The constructor returns a FlowNode node.

## Prototype

```cpp
FlowNode(const char *name, uint32_t input_num, uint32_t output_num)
~FlowNode() override
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the compute node, which must be unique in the entire graph. |
| input_num | Input | Number of inputs of the node. |
| output_num | Input | Number of outputs of the node. |

## Return Value

Returns a FlowNode node.

## Exception Handling

None.

## Constraints

None.
