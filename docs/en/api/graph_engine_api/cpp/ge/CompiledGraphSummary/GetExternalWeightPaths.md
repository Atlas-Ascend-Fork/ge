# GetExternalWeightPaths

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:20:32.998Z pushedAt=2026-10-28T07:40:22.282Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the disk path for external weights.

## Prototype

```c++
Status GetExternalWeightPaths(std::vector<ExternalWeightDescPtr> &paths) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| paths | Output | Path of the **shared_ptr** structure of [ExternalWeightDesc](../ExternalWeightDesc/ExternalWeightDesc.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: The API is called successfully.<br>  - FAILED: The API call fails. |

## Constraints

None
