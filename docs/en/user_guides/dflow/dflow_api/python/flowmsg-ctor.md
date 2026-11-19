# FlowMsg Constructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:28:59.377Z pushedAt=2026-11-16T07:55:49.470Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Constructor of FlowMsg.

## Prototype

```python
__init__(self, flow_msg: flowfunc_wrapper.FlowMsg) -> None
```

## Parameters

FlowMsg defined in the flowfunc\_wrapper module. During actual execution, it is passed in by the C++ code and mapped to the flowfunc\_wrapper FlowMsg object through the pybind11 binding.

## Return Value

Returns an object of the FlowMsg type.

## Exception Handling

None

## Constraints

None
