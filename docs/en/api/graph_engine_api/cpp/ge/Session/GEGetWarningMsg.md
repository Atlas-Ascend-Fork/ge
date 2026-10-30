# GEGetWarningMsg

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:15:33.552Z pushedAt=2026-10-28T07:42:19.801Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Obtains and clears the warning messages generated when other APIs in the same process or thread as this API fail to be called.

When warning messages need to be viewed after program execution, call this API to print the warning messages. This API cannot be used together with [GEGetWarningMsgV2](GEGetWarningMsgV2.md), and [GEGetWarningMsgV2](GEGetWarningMsgV2.md) is recommended.

## Prototype

```c++
std::string GEGetWarningMsg()
```

## Parameters

None

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | std::string | Warning messages generated during execution. |

## Constraints

None
