# REG\_FUSION\_PASS

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:21:55.240Z pushedAt=2026-10-28T07:40:22.528Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <ge/fusion/pass/pattern\_fusion\_pass.h\>

## Description

Developers can register the graph modification function with the framework, which invokes the custom graph modification Pass at the very beginning of compilation. Call **REG\_FUSION\_PASS** to register the custom Pass.

## Prototype

```c++
REG_FUSION_PASS(pass_class)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pass_class | Input | Name of the custom Pass. |

## Return Value

None

## Constraints

None
