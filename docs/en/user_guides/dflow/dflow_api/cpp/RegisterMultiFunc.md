# RegisterMultiFunc

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:46:08.853Z pushedAt=2026-11-16T05:54:07.653Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Registers a multi-flow func.

It is not recommended to use this function directly. You are advised to use the [MetaMultiFunc Registration Function Macro](metamultifunc-register-func-macro.md) to register a flow func.

## Prototype

- Registers a common flow func, which is used when the flow func input is flowMsg.

    ```cpp
    bool RegisterMultiFunc(const char *flowFuncName, const MULTI_FUNC_CREATOR_FUNC &funcCreator) noexcept
    ```

- Registers a streaming-input flow func (that is, the function input parameter is a queue), which is used when the flow func input is flowMsgQueue.

    ```cpp
    bool RegisterMultiFunc(const char *flowFuncName, const MULTI_FUNC_WITH_Q_CREATOR_FUNC &funcWithQCreator) noexcept
    ```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| flowFuncName | Input | Name of the flow func. It cannot be set to NULL and must end with **\0**. |
| funcCreator/funcWithQCreator | Input | Creation function of the multi-flow func. |

## Return Value

- true
- false

## Exception Handling

None.

## Constraints

None.
