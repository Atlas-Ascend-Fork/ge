# GEGetErrorMsgV3

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:47:22.961Z pushedAt=2026-10-28T07:40:22.642Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Obtains and clears the error description information generated when other APIs fail in the same process or thread as this API.

It is recommended to call this API when an error occurs to obtain the error information and help locate the problem.

## Prototype

```c++
ge::AscendString GEGetErrorMsgV3()
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::AscendString | Error information generated during execution. |

## Constraints

None
