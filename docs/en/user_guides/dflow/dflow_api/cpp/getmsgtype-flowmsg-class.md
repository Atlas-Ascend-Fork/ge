# GetMsgType (FlowMsg Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T02:56:48.427Z pushedAt=2026-11-16T03:39:31.258Z nl=2 -->

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

```cpp
MsgType GetMsgType() const
```

## Parameters

None

## Return Value

Returns the message type of FlowMsg.

```cpp
enum class MsgType : uint16_t {
    MSG_TYPE_TENSOR_DATA = 0,          // tensor data msg type
    MSG_TYPE_RAW_MSG = 1,              // raw data msg type
    MSG_TYPE_TENSOR_LIST = 2,          // raw data msg type
    MSG_TYPE_USER_DEFINE_START = 1024  // user define type start
};
```

## Exception Handling

None.

## Constraints

None.
