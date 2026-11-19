# aclgrphBuildInitialize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:03:13.201Z pushedAt=2026-10-28T07:40:22.105Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_ir\_build.h\>
- Library file: libge\_compiler.so

## Description

Initialization function for model build, which is used to allocate resources.

## Prototype

> [!NOTE]
> The string-type API will be deprecated in future versions. You are advised to use the non-string type instead.

```c++
graphStatus aclgrphBuildInitialize(std::map<std::string, std::string> global_options)
graphStatus aclgrphBuildInitialize(std::map<AscendString, AscendString> &global_options)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| global_options | Input | Global configuration parameters.<br>A map of configuration parameters, where the key is the parameter type and the value is the parameter value, both in string format, used to describe the initialization information for offline model compilation.<br>For the configuration parameters supported in the map, see [Configuration Parameters Supported by aclgrphBuildInitialize](./aclgrphBuildInitialize_config_params/aclgrphbuildinitialize_config_params.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Constraints

The **aclgrphBuildInitialize** API can be called only once within a process.

After this API is called, the [aclgrphBuildModel](aclgrphBuildModel.md) API can be called multiple times consecutively within the same process.

**For the duplicate options compilation configuration parameters in aclgrphBuildModel and aclgrphBuildInitialize, it is recommended not to configure them repeatedly. If they are configured repeatedly, the values passed to aclgrphBuildModel prevail**.
