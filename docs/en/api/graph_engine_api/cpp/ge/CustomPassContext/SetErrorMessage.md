# SetErrorMessage

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:27:47.490Z pushedAt=2026-10-28T07:40:22.308Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Registers the error message encountered during the graph modification phase with the framework.

## Prototype

```c++
void SetErrorMessage(const AscendString &error_message)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| error_message | Input | Error message to be registered with the framework. |

## Return Value

None

## Constraints

None
