# AddPp

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:35:15.053Z pushedAt=2026-10-28T12:30:42.431Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: supported
<!-- end id2 -->

## Function

Adds a mapped ProcessPoint to a FlowNode. Currently, a FlowNode can be mapped to only one ProcessPoint. After the ProcessPoint is added, the inputs and outputs of the FlowNode are mapped to those of the ProcessPoint in sequence by default.

## Prototype

```cpp
FlowNode &AddPp(const ProcessPoint &pp)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pp | Input | ProcessPoint mapped to the FlowNode. The ProcessPoint can be a [FunctionPp Class](functionpp-class.md) or a [ProcessPoint Class](processpoint-class.md). |

## Return Value

Returns a FlowNode node.

## Exception Handling

None.

## Constraints

The number of inputs/outputs of the added ProcessPoint must be consistent with that of the FlowNode.
