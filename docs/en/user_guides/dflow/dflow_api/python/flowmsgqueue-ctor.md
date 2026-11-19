# FlowMsgQueue Constructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:29:58.458Z pushedAt=2026-11-16T07:56:17.112Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

FlowMsgQueue constructor and destructor.

## Prototype

```python
__init__(self, flow_msg_queue: flowfunc_wrapper.FlowMsgQueue) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| flowfunc_wrapper.FlowMsgQueue | Input | FlowMsgQueue defined in the flowfunc_wrapper module. During actual execution, it is passed in by C++ code and mapped to a FlowMsgQueue object of flowfunc_wrapper through the pybind11 binding relationship. |

## Return Value

Returns an object of the FlowMsgQueue type.

## Exception Handling

None

## Constraints

Inherits the queue.Queue type of Python, but supports only five interfaces: get, get\_nowait, full, empty, and qsize.

In streaming input scenarios (where the flow func function takes a queue as its input parameter), DataFlow does not support data alignment or proactive exception reporting by UDFs.
