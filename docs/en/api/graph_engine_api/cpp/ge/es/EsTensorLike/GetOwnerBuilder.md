# GetOwnerBuilder

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:16:16.326Z pushedAt=2026-10-28T07:40:22.501Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_like.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the owner builder of the **Tensor** corresponding to the current **TensorLike**.

## Prototype

```c++
EsCGraphBuilder *GetOwnerBuilder() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCGraphBuilder * | **EsCGraphBuilder** pointer. |

## Constraints

None

## Examples

```c++
EsTensorLike tensor_like(tensor);
auto *owner_builder = tensor_like.GetOwnerBuilder();
```
