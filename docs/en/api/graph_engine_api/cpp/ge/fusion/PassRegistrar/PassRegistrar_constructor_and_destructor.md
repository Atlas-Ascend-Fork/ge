# PassRegistrar Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:31:00.915Z pushedAt=2026-10-28T07:40:22.569Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <ge/fusion/pass/fusion\_pass\_reg.h\>

## Description

**PassRegistrar** constructor and destructor.

## Prototype

```c++
PassRegistrar(FusionPassRegistrationData &fusion_pass_reg_data)
~PassRegistrar() = default
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| fusion_pass_reg_data | Input | Fusion pass registration data of the [FusionPassRegistrationData](../FusionPassRegistrationData/FusionPassRegistrationData.md) class. |

## Return Value

None

## Constraints

None
