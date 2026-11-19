# RegisterCallBackFunc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:19:45.570Z pushedAt=2026-10-28T07:42:19.815Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Registers a callback function.

Registers the user-specified Summary and Checkpoint callback APIs. When the graph delivered by you to GE contains Summary and Checkpoint operators, GE calls the callback function.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. You are advised to use the API with the non-string data type.

```c++
Status RegisterCallBackFunc(const std::string &key, const pCallBackFunc &callback)
Status RegisterCallBackFunc(const char *key, const session::pCallBackFunc &callback)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| key | Input | Keyword corresponding to the registered callback function, in string or character format, indicating the callback function type. "Summary" and "Save" are supported. |
| callback | Input | Information returned by the callback function. For details, see the content below the table. |

```c++
typedef uint32_t (*pCallBackFunc)(uint32_t graph_id, const std::map<std::string, ge::Tensor> &params_list);
typedef uint32_t (*pCallBackFunc)(uint32_t graph_id, const std::map<AscendString, ge::Tensor> &params_list);
```

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **GE_SESSION_MANAGER_NOT_INIT**: Session management is not initialized.<br>**SUCCESS**: The callback function is registered successfully.<br>**FAILED**: Failed to register the callback function. |

## Constraints

- The callback function type supports only Summary and Save.
- If no callback is registered, delivering Summary and Checkpoint operators will report an error.
- Currently, the callback function is only supported to be invoked once after the graph execution completes.
