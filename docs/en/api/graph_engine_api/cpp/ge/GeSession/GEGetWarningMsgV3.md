# GEGetWarningMsgV3

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:47:51.904Z pushedAt=2026-10-28T07:40:22.645Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Obtains and clears the warning message generated when other APIs in the same process or thread as this API fail to be called.

## Prototype

```c++
ge::AscendString GEGetWarningMsgV3()
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::AscendString | Warning message generated during execution. |

## Constraints

None
