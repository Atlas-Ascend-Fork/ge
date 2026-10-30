# CreatePassFn

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:23:20.634Z pushedAt=2026-10-28T07:40:22.534Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pass/fusion\_pass\_reg.h\>
- Library file: libge\_compiler.so

## Description

Creates a fusion pass registration function.

## Prototype

```c++
FusionPassRegistrationData &CreatePassFn(const CreateFusionPassFn &create_fusion_pass_fn)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| create_fusion_pass_fn | Input | Execution function for creating a custom pass. |

## Return Value

A reference to **FusionPassRegistrationData**, used for cascading calls to its member functions.

## Constraints

None

## Callback Function CreateFusionPassFn

The user custom-implements a fusion pass subclass and invokes the registration macro:

```c++
REG_DECOMPOSE_PASS/
REG_FUSION_PASS
```

The registration macro automatically generates a callback function that constructs the fusion pass subclass object, namely **CreateFusionPassFn**. This callback function is used to construct the **Pass** object before the pass is executed:

```c++
using CreateFusionPassFn = FusionBasePass *(*)();
```
