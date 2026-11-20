# GEGetWarningMsgV2

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:15:56.748Z pushedAt=2026-10-28T07:42:19.802Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Obtains and clears the warning messages generated when other APIs in the same process or thread as this API fail to be called.

When you need to view warning messages after program execution ends, call this API to print the warning messages. This is an ABI-compatible API (corresponding to **D\_GLIBCXX\_USE\_CXX11\_ABI** in the compiled file) and returns **ge::AscendString**. It cannot be used together with [GEGetWarningMsg](GEGetWarningMsg.md). This API is recommended.

## Prototype

```c++
ge::AscendString GEGetWarningMsgV2()
```

## Parameters

None

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | ge::AscendString | Warning messages generated during execution. |

## Constraints

None
