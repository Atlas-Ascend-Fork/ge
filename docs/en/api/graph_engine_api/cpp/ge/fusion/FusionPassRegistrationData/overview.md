# Overview

<!-- md-trans-meta sourceCommit=351a747bcd465b5e5848654d26a0978a5d08a8cd translatedAt=2026-10-16T13:26:07.148Z pushedAt=2026-10-28T07:40:22.543Z nl=2 -->

A class that stores the registration information of a fusion Pass.

## Required Header File

```c++
#include <ge/fusion/pass/fusion_pass_reg.h>
```

## Public Member Functions

```c++
FusionPassRegistrationData() = default
explicit FusionPassRegistrationData(const AscendString &pass_name)
AscendString GetPassName() const
FusionPassRegistrationData &Stage(CustomPassStage stage)
CustomPassStage GetStage() const
FusionPassRegistrationData &CreatePassFn(const CreateFusionPassFn &create_fusion_pass_fn)
CreateFusionPassFn GetCreatePassFn() const
AscendString ToString() const
```
