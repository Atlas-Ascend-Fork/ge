# ResolveBuilderImpl

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:17:26.357Z pushedAt=2026-10-28T07:40:22.506Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_like.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the owner builder.

## Prototype

- Resolves the owning **EsCGraphBuilder** from an **EsTensorLike** object.

    ```c++
    EsCGraphBuilder *ResolveBuilderImpl(const EsTensorLike &tensor_like)
    ```

- Resolves the owning **EsCGraphBuilder** from a vector of **EsTensorHolder** objects.

    ```c++
    EsCGraphBuilder *ResolveBuilderImpl(const std::vector<EsTensorHolder> &tensors)
    ```

- Resolves the owning **EsCGraphBuilder** from **EsCTensorHolder**\*.

    ```c++
    EsCGraphBuilder *ResolveBuilderImpl(EsCTensorHolder *esb_tensor)
    ```

- Resolves the owning **EsCGraphBuilder** from a vector of **EsCTensorHolder**\*.

    ```c++
    EsCGraphBuilder *ResolveBuilderImpl(const std::vector<EsCTensorHolder *> &esb_tensors)
    ```

- Obtains the underlying **EsCGraphBuilder** from an **EsGraphBuilder** pointer.

    ```c++
    EsCGraphBuilder *ResolveBuilderImpl(const EsGraphBuilder *graph_builder)
    ```

- Directly returns the parameter that is already **EsCGraphBuilder**\*.

    ```c++
    EsCGraphBuilder *ResolveBuilderImpl(EsCGraphBuilder *graph_builder)
    ```

- Implementation for processing the **nullptr\_t** parameter.

    ```c++
    EsCGraphBuilder *ResolveBuilderImpl(const std::nullptr_t)
    ```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor_like | Input | **EsTensorLike** object. |
| tensors | Input | **EsTensorHolder** vector. |
| esb_tensor | Input | **EsCTensorHolder** pointer vector. |
| graph_builder | Input | Graph builder pointer. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCGraphBuilder * | C-layer builder pointer. |

## Constraints

None
