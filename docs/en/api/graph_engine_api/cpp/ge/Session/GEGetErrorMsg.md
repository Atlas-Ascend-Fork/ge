# GEGetErrorMsg

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:14:30.244Z pushedAt=2026-10-28T07:42:19.797Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Retrieves and clears the error description information generated when other APIs in the same process or thread as this API fail to be called.

It is recommended to call this API when an error occurs to obtain the error information for problem locating. This API cannot be used together with [GEGetErrorMsgV2](GEGetErrorMsgV2.md); [GEGetErrorMsgV2](GEGetErrorMsgV2.md) is recommended.

## Prototype

```c++
std::string GEGetErrorMsg()
```

## Parameters

None

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | std::string | Error information generated during execution. |

## Constraints

None
