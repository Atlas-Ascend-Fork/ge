# CreateMultiFuncWithQ

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:14:20.615Z pushedAt=2026-11-16T06:11:08.706Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Creates a multi-func processing object and processing function whose input type is [FlowMsgQueue](flowmsgqueue-class.md). This API is used internally by the framework and is not directly used by users.

## Prototype

Creates a flow func processing object and processing function with streaming input (that is, the function input is a queue), specifically when the flow func input is flowMsgQueue.

```cpp
int32_t CreateMultiFuncWithQ(std::shared_ptr<MetaMultiFunc> &multiFunc,
std::map<AscendString, PROC_FUNC_WITH_CONTEXT_Q> &procFuncWithQMap) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| multiFunc | Output | Multi-func instance object |
| procFuncWithQMap | Output | Processing function of the multi-flow func whose input type is FlowMsgQueue. |

## Return Value

- FLOW\_FUNC\_SUCCESS: success
- Other values indicate failure.

## Exception Handling

None.

## Constraints

None.
