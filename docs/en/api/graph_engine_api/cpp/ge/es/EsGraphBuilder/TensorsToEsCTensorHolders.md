# TensorsToEsCTensorHolders

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:09:56.876Z pushedAt=2026-10-28T07:40:22.478Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Converts an **EsTensorHolder** vector into an **EsCTensorHolder** pointer vector.

## Prototype

```c++
inline std::vector<EsCTensorHolder *> TensorsToEsCTensorHolders(const std::vector<EsTensorHolder> &tensors)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensors | Input | **EsTensorHolder** vector. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::vector<EsCTensorHolder *> | Returns an **EsCTensorHolder** pointer vector. |

## Constraints

None
