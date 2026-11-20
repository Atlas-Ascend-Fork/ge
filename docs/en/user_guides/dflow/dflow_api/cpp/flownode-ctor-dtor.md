# FlowNode Constructor and Destructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:29:49.489Z pushedAt=2026-11-16T06:11:08.767Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

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
