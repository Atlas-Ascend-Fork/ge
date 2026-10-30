# REGISTER\_CUSTOM\_PASS

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:29:25.703Z pushedAt=2026-10-28T07:40:22.184Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <register/register\_custom\_pass.h\>

## Description

You can register a graph modification function with the framework, which invokes the custom graph modification pass at the very beginning of compilation. Call **REGISTER\_CUSTOM\_PASS** to register the custom pass.

## Prototype

```c++
REGISTER_CUSTOM_PASS(name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the custom pass. |

## Return Value

None

## Constraints

The call starts with **REGISTER\_CUSTOM\_PASS** and connects interfaces such as **CustomPassFn** with ".". For example:

```c++
#include "register/register_custom_pass.h"
REGISTER_CUSTOM_PASS("pass_name").CustomPassFn(CustomPassFunc);
```
