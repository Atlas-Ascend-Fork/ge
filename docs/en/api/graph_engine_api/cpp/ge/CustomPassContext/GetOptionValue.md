# GetOptionValue

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:26:42.536Z pushedAt=2026-10-28T07:40:22.303Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Obtains the value of an **option** from the context by its **key**.

## Prototype

```c++
graphStatus GetOptionValue(const AscendString &option_key, AscendString &option_value) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| option_key | Input | Key of the **option**. |
| option_value | Output | Value of the **option**. |

## Return Value

Status code. If the **option key** does not exist, a failure value is returned.

## Constraints

None
