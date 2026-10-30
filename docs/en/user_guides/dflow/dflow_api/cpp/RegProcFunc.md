# RegProcFunc

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:08:20.518Z pushedAt=2026-10-29T02:05:07.285Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Registers multiple flow func processing functions, and registers flow func in combination with the [MetaMultiFunc Registration Function Macro](metamultifunc-register-func-macro.md).

## Prototype

- Registers a common flow func, which is used when the flow func input is flowMsg.

    ```cpp
    FlowFuncRegistrar &RegProcFunc(const char *flowFuncName, const CUSTOM_PROC_FUNC &func)
    ```

- Registers a streaming-input flow func (that is, the function input parameter is a queue), which is used when the flow func input is flowMsgQueue.

    ```cpp
    FlowFuncRegistrar &RegProcFunc(const char *flowFuncName, const CUSTOM_PROC_FUNC_WITH_Q &func)
    ```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| flowFuncName | Input | Name of the flow func. It cannot be set to NULL and must end with “\0”. |
| func | Input | Processing function of the multiple flow func. The processing function prototype is<br>`using CUSTOM_PROC_FUNC = std::function<int32_t(<br>T *, const std::shared_ptr<MetaRunContext> &, const std::vector<std::shared_ptr<FlowMsg>> &)>;`<br>or<br>`using CUSTOM_PROC_FUNC_WITH_Q = std::function<int32_t(<br>T*, const std::shared_ptr<MetaRunContext> &, const std::vector<std::shared_ptr<FlowMsgQueue>> &)>;` |

## Return Value

Returns the current FlowFuncRegistrar class object.

## Exception Handling

None.

## Constraints

None.
