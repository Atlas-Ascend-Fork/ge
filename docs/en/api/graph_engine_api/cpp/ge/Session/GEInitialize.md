# GEInitialize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:16:22.364Z pushedAt=2026-10-28T07:42:19.803Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Initializes GE and prepares it for running.

## Prototype

> [!NOTE]
> APIs whose data type is string will be deprecated in later versions. You are advised to use APIs whose data type is not string.

```c++
Status GEInitialize(const std::map<std::string, std::string> &options)
Status GEInitialize(const std::map<AscendString, AscendString> &options)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| options | Input | A map whose keys are parameter types and values are parameter values, both in string format. It describes the configuration information related to initialization. For the currently supported configuration information, see the global-level parameters in [options Parameter Description](../options_params/options_parameters_description.md). |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **0**: Initialization succeeded.<br>Other values: Initialization failed. |

## Constraints

- GE does not support multi-instance running. Only one instance can be initialized at a time.
- Calling this API multiple times without calling GEFinalize results in unpredictable behavior.
