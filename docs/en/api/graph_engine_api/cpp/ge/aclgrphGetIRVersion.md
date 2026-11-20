# aclgrphGetIRVersion

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:06:59.444Z pushedAt=2026-10-28T07:40:22.116Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_ir\_build.h\>
- Library file: libge\_compiler.so

## Description

Obtains the version number of the model building related APIs.

## Prototype

```c++
graphStatus aclgrphGetIRVersion(int32_t *major_version, int32_t *minor_version, int32_t *patch_version)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **major_version** | Output | Major version number. |
| **minor_version** | Output | Minor version number. |
| **patch_version** | Output | Bug fix version number. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | **graphStatus** | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

It is recommended to call the API with integer reference parameters. If an integer null pointer is passed in, the API returns a failure after null check.
