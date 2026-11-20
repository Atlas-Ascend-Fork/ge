# CreateMultiFunc

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:14:14.746Z pushedAt=2026-11-16T06:11:08.703Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Creates a multi-func processing object and handler function. This function is used internally by the framework and is not directly used by users.

## Prototype

Creates a common flow func processing object and processing function, that is, when the flow func input is flowMsg.

```cpp
int32_t CreateMultiFunc(std::shared_ptr<MetaMultiFunc> &multiFunc,
std::map<AscendString, PROC_FUNC_WITH_CONTEXT> &procFuncMap) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| multiFunc | Output | Multi-func instance object |
| procFuncMap | Output | Processing function of the multi-flow func. |

## Return Value

- FLOW\_FUNC\_SUCCESS: success
- Other values indicate failure.

## Exception Handling

None.

## Constraints

None.
