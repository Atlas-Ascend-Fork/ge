# GEGetErrorMsgV2

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:15:10.400Z pushedAt=2026-10-28T07:42:19.799Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Obtains and clears the error description generated when other APIs in the same process or thread as this API fail to be called.

This API is an ABI-compatible API (corresponding to **D\_GLIBCXX\_USE\_CXX11\_ABI** in the compiled file). It returns **ge::AscendString** and cannot be used together with [GEGetErrorMsg](GEGetErrorMsg.md). This API is recommended.

## Prototype

```c++
ge::AscendString GEGetErrorMsgV2()
```

## Parameters

None

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | ge::AscendString | Error message generated during execution. |

## Constraints

It is recommended to call this API when an error occurs during execution to obtain the error message for problem locating.
