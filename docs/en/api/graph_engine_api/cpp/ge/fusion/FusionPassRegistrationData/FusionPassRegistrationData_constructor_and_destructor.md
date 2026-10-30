# FusionPassRegistrationData Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:23:51.562Z pushedAt=2026-10-28T07:40:22.538Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pass/fusion\_pass\_reg.h\>
- Library file: libge\_compiler.so

## Description

Constructor and destructor of **FusionPassRegistrationData**.

## Prototype

```c++
FusionPassRegistrationData() = default
~FusionPassRegistrationData() = default
explicit FusionPassRegistrationData(const AscendString &pass_name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pass_name | Input | Fusion pass name. |

## Return Value

None

## Constraints

None
