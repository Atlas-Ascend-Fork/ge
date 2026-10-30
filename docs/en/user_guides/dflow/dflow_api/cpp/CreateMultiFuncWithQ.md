# CreateMultiFuncWithQ

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:45:21.348Z pushedAt=2026-10-28T12:30:42.470Z nl=2 -->

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

It is used to create a flow func processing object and processing function with streaming input (that is, the function input is a queue), specifically when the flow func input is flowMsgQueue.

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
