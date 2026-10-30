# CreateMultiFunc

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:44:59.910Z pushedAt=2026-10-28T12:30:42.469Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Creates a multi-func processing object and processing function. This API is used internally by the framework and is not directly used by users.

## Prototype

Used to create a common flow func processing object and processing function, that is, when the flow func input is flowMsg.

```cpp
int32_t CreateMultiFunc(std::shared_ptr<MetaMultiFunc> &multiFunc,
std::map<AscendString, PROC_FUNC_WITH_CONTEXT> &procFuncMap) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| multiFunc | Output | Multi-func instance object |
| procFuncMap | Output | Processing function of the multi-flow func. |

## Return Values

- FLOW\_FUNC\_SUCCESS: Success
- Other values indicate failure

## Exception Handling

None.

## Constraints

None.
