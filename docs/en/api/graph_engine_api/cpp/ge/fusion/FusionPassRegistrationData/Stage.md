# Stage

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:26:30.595Z pushedAt=2026-10-28T07:40:22.546Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pass/fusion\_pass\_reg.h\>
- Library file: libge\_compiler.so

## Description

Defines the fusion pass stage.

Note: The **kAfterAssignLogicStream** stage does not allow registration of ordinary fusion passes. This stage does not permit modification of the graph structure. If a pass is mistakenly registered to this stage, it will be ignored.

## Prototype

```c++
FusionPassRegistrationData &Stage(CustomPassStage stage)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| stage | Input | Execution stage of the custom fusion pass. For details, see [CustomPassStage](../../CustomPassStage.md). |

## Return Value

Returns the **FusionPassRegistrationData** object.

## Constraints

None
