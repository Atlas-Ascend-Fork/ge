# FlowMsg Constructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:43:36.863Z pushedAt=2026-10-29T03:10:03.805Z nl=2 -->

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

flowfunc\_wrapper FlowMsg defined in the module. During actual execution, it is passed in by the C++ code and mapped to the flowfunc\_wrapper FlowMsg object through the pybind11 binding.

## Return Value

Returns an object of the FlowMsg type.

## Exception Handling

None

## Constraints

None
