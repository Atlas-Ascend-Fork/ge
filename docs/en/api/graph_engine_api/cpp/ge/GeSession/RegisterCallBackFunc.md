# RegisterCallBackFunc

<!-- md-trans-meta sourceCommit=f466c256e1a282548a091433bcbec11edd4d9538 translatedAt=2026-10-16T13:52:30.243Z pushedAt=2026-10-28T07:40:22.661Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Registers a callback function.

Registers the user-specified Summary and Checkpoint callback APIs. When the graph delivered by the user to GE contains Summary and Checkpoint operators, GE invokes this callback function.

## Prototype

```c++
Status RegisterCallBackFunc(const char *key, const RunCallback &callback)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| key | Input | Keyword corresponding to the registered callback function, in string or character format, indicating the callback function type. **Summary** and **Save** are supported. |
| callback | Input | Information returned by the callback function. For details, see the RunCallback content below the table. |

```c++
using RunCallback = std::function<Status(uint32_t graph_id, const std::map<AscendString, gert::Tensor> &params_list)>;
```

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **GE_SESSION_MANAGER_NOT_INIT**: Session management is not initialized.<br>**SUCCESS**: The callback function is registered successfully.<br>**FAILED**: Failed to register the callback function. |

## Constraints

- Only **Summary** and **Save** are supported as callback function types.
- If not registered, delivering Summary and Checkpoint operators will report an error.
- Currently, the callback function is invoked only once after graph execution is complete.
