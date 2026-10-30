# IsIrRepSupport

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:36:11.316Z pushedAt=2026-10-28T13:41:49.801Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Feature Description

Queries the IR (Intermediate Representation) expression capability.

## Prototype

```c
bool IsIrRepSupport(const char *rep)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| rep | Input | IR expression capability. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | bool | true: query succeeded.<br>false: query failed. |

## Constraints

None

## Examples

```c
#include "ge/ge_api_v2.h"
bool existingFeature = IsIrRepSupport("_inference_rule");
```
