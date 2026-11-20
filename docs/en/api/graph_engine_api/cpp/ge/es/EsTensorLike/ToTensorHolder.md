# ToTensorHolder

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:17:43.044Z pushedAt=2026-10-28T07:40:22.510Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_like.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Converts the current **EsTensorLike** into an **EsTensorHolder**.

## Prototype

```c++
EsTensorHolder ToTensorHolder(EsCGraphBuilder *graph) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | The graph builder to which it belongs. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsTensorHolder | The converted **EsTensorHolder** object. |

## Constraints

None

## Examples

```c++
EsTensorLike tensor_like(tensor);
auto result = tensor_like.ToTensorHolder(c_builder);
```
