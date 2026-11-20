# MapInput

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:19:30.464Z pushedAt=2026-11-16T04:53:46.497Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Maps the input of a FlowNode, indicating that the node\_input\_index-th input of the FlowNode is passed to the pp\_input\_index-th input of the ProcessPoint, and sets all properties in attrs for this input of the ProcessPoint. Returns the FlowNode node after mapping. This function is optional. If it is not called, the inputs of the FlowNode and the ProcessPoint are mapped in order by default.

## Prototype

```cpp
FlowNode &MapInput(uint32_t node_input_index, const ProcessPoint &pp, uint32_t pp_input_index, const std::vector<DataFlowInputAttr> &attrs = {})
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node_input_index | Input | Input index of the FlowNode. |
| pp | Input | ProcessPoint to which the FlowNode node is mapped. |
| pp_input_index | Input | Index of the ProcessPoint input. |
| attrs | Input | Attribute set. For details about the attribute values, see [DataFlowInputAttr Struct](dataflowinputattr-struct.md). |

## Return Value

Returns the FlowNode node whose input has been mapped.

## Exception Handling

None.

## Constraints

Before calling this API, you must call [FlowNode &AddPp](AddPp.md).
