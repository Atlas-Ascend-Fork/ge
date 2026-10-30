# REG\_DECOMPOSE\_PASS

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:28:32.646Z pushedAt=2026-10-28T07:40:22.179Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <ge/fusion/pass/decompose\_pass.h\>

## Description

You can register the graph modification function for 1-to-N scenarios with the framework, which invokes the custom graph modification Pass at the very beginning of compilation by calling **REG\_DECOMPOSE\_PASS** for custom Pass registration.

## Prototype

```c++
REG_DECOMPOSE_PASS(pass_class, decompose_op_types)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pass_class | Input | Name of the custom Pass. |
| decompose_op_types | Input | Operator types to be replaced. |

## Return Value

None

## Constraints

None
