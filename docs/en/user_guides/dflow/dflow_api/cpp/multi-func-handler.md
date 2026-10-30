# Multi-Func Processing Function

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:03:24.044Z pushedAt=2026-10-29T02:01:50.848Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

A user-defined multi-flow func handler function.

## Prototype

- In normal scenarios, the function input is prepared by the framework and then directly provided to the user, who can directly use the flowMsg in the input parameter.

    ```cpp
    std::function<int32_t> (const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &flowMsg)
    ```

- In streaming input scenarios (where the function input parameter is a queue), the user obtains data from the input queue for use.

    ```cpp
    std::function<int32_t> (const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsgQueue>> &flowMsgQueue)
    ```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| runContext | Input | Context information of the handler function. |
| FlowMsg/FlowMsgQueue | Input | Input parameter/input queue of the function. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

- If an unrecoverable exception occurs, return ERROR.
- In other cases, call SetRetCode to set the error code of the output tensor.
- If success is returned, scheduling is terminated.

## Constraints

When using a streaming input flow func, set the stream\_input field of the corresponding func to true in the ProcessPoint compilation configuration file. For example:

```cpp
{
    "func_list": [
        {
            "func_name": "Func",
            "stream_input": true
        }
    ],
    "input_num": 1,
    "output_num": 1,
    "target_bin": "libfunc.so",
    "workspace": "./",
    "cmakelist_path": "CMakeLists.txt",
}
```
