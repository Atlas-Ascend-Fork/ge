# AddPp

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:05:01.121Z pushedAt=2026-11-16T06:11:08.667Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Adds a mapped ProcessPoint to a FlowNode. Currently, a FlowNode can have only one ProcessPoint added. After the ProcessPoint is added, the inputs and outputs of the FlowNode are mapped to those of the ProcessPoint in sequence by default.

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
