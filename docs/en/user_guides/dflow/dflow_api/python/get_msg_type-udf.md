# get\_msg\_type (UDF)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:43:23.913Z pushedAt=2026-11-16T08:02:08.675Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the message type of FlowMsg.

## Prototype

```python
get_msg_type(self)
```

## Parameters

None

## Return Value

Returns the message type of FlowMsg.

```python
import dataflow.flow_func.flow_func as ff
# The message returns the following two values:
ff.MSG_TYPE_TENSOR_DATA
ff.MSG_TYPE_RAW_MSG
# A value greater than or equal to 1024 indicates a user-defined type.
```

## Exception Handling

None

## Constraints

None
