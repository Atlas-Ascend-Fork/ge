# GEInitializeV2

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:48:05.689Z pushedAt=2026-10-28T07:40:22.648Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Initializes GE and completes the preparation for running.

## Prototype

```c++
Status GEInitializeV2(const std::map<AscendString, AscendString> &options)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| options | Input | A map table where the key is the parameter type and the value is the parameter value, both in string format, describing initialization-related configuration information. For the currently supported configuration information, see the global-level parameters in [options Parameter Description](../options_params/options_parameters_description.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | 0: Initialization succeeded.<br>Other values: Initialization failed. |

## Constraints

- GE does not support multi-instance running. Only one instance can be initialized at a time.
- If this API is called multiple times without calling [GEFinalizeV2](GEFinalizeV2.md), the behavior is unpredictable.
