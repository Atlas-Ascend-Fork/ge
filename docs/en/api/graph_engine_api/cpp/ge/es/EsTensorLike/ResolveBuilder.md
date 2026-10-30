# ResolveBuilder

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:16:52.507Z pushedAt=2026-10-28T07:40:22.504Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_like.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Returns **nullptr** when no parameter is provided, or obtains the **owner builder** from a **Tensor** object/vector.

## Prototype

- Returns **nullptr** when no parameter is provided.

    ```c++
    EsCGraphBuilder *ResolveBuilder()
    ```

- Obtains the **owner builder** from a **Tensor** object/vector.

    ```c++
    EsCGraphBuilder *ResolveBuilder(const T& first, const Ts&... rest)
    ```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| first | Input | **Tensor** object/vector. |
| rest | Input | Remaining **Tensor** objects/vectors. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCGraphBuilder * | - Prototype 1: Returns **nullptr**.<br>  - Prototype 2: Returns the C-layer **builder** pointer. |

## Constraints

None
