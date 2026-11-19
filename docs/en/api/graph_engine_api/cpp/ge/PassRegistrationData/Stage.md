# Stage

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:09:59.661Z pushedAt=2026-10-28T07:42:19.779Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Sets the execution stage of a custom Pass.

## Prototype

```c++
PassRegistrationData &Stage(const CustomPassStage stage)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| stage | Input | Execution stage of the custom Pass. For details, see [CustomPassStage](../CustomPassStage.md). |

## Return Value

Returns the **PassRegistrationData** object.

## Constraints

None
