# RegisterMultiFunc

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:07:56.787Z pushedAt=2026-10-29T02:04:34.036Z nl=2 -->

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

- Registers a streaming-input flow func (that is, the function input is a queue), which is used when the flow func input is flowMsgQueue.

    ```cpp
    bool RegisterMultiFunc(const char *flowFuncName, const MULTI_FUNC_WITH_Q_CREATOR_FUNC &funcWithQCreator) noexcept
    ```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| flowFuncName | Input | Name of the flow func. It cannot be set to NULL and must end with “\0”. |
| funcCreator/funcWithQCreator | Input | Creation function of the multi-flow func. |

## Return Value

- true
- false

## Exception Handling

None.

## Constraints

None.
