# Overview

<!-- md-trans-meta sourceCommit=351a747bcd465b5e5848654d26a0978a5d08a8cd translatedAt=2026-10-26T13:08:53.354Z pushedAt=2026-10-28T07:42:19.775Z nl=2 -->

A class used to register and manage custom Passes (processing stages).

## Header File to Include

```c++
#include <register/register_custom_pass.h>
```

## Public Member Functions

```c++
PassRegistrationData() = default
~PassRegistrationData() = default
PassRegistrationData(std::string pass_name)
PassRegistrationData &CustomPassFn(const CustomPassFunc &custom_pass_fn)
std::string GetPassName() const
CustomPassFunc GetCustomPassFn() const
PassRegistrationData &Stage(const CustomPassStage stage)
CustomPassStage GetStage() const
PassRegistrationData &CustomAllocateStreamPassFn(const CustomAllocateStreamPassFunc &allocate_stream_pass_fn)
CustomAllocateStreamPassFunc GetCustomAllocateStreamPass() const
```
