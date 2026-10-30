# Session Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:23:06.378Z pushedAt=2026-10-28T07:42:19.825Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Session constructor and destructor.

## Prototype

> [!NOTE]
> APIs whose data type is string will be deprecated in later versions. You are advised to use APIs whose data type is not string.

```c++
explicit Session(const std::map<std::string, std::string> &options)
explicit Session(const std::map<AscendString, AscendString> &options)
~Session()
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| options | Input | A map table whose keys are parameter types and values are parameter values, both in string format, describing the configuration information of the Session.<br>Generally, this parameter can be left empty, in which case the global options passed to **GEInitialize** are used.<br>To configure the current Session separately, use this parameter. For supported configuration items, see the session-level parameters in [Options Parameter Description](../options_params/options_parameters_description.md). |

## Return Value

None

## Constraints

A Session does not support concurrent execution and exclusively occupies resources. When multiple Sessions are created at the same time, Session creation may fail due to insufficient resources.
