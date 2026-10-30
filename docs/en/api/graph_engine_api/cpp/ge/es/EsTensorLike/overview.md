# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:16:45.375Z pushedAt=2026-10-28T07:40:22.503Z nl=2 -->

A generic **EsTensorHolder** type that supports converting scalar and vector values into **EsTensorHolder** objects, making it convenient for C++ graph construction APIs to directly pass values for graph construction.

## Header File to Include

```c++
#include <es_tensor_like.h>
```

## Public Member Functions

```c++
EsTensorLike(const EsTensorHolder &tensor)
EsTensorLike(const std::nullptr_t)
EsTensorLike(const int64_t value)
EsTensorLike(const float value)
EsTensorLike(const std::vector<int64_t> &values)
EsTensorLike(const std::vector<float> &values)
~EsTensorLike()
EsCGraphBuilder *GetOwnerBuilder() const
EsTensorHolder ToTensorHolder(EsCGraphBuilder *graph) const
```

## External Functions

```c++
EsCGraphBuilder *ResolveBuilderImpl(const EsTensorLike &tensor_like)
EsCGraphBuilder *ResolveBuilderImpl(const std::vector<EsTensorHolder> &tensors)
EsCGraphBuilder *ResolveBuilderImpl(EsCTensorHolder *esb_tensor)
EsCGraphBuilder *ResolveBuilderImpl(const std::vector<EsCTensorHolder *> &esb_tensors)
EsCGraphBuilder *ResolveBuilderImpl(const EsGraphBuilder *graph_builder)
EsCGraphBuilder *ResolveBuilderImpl(EsCGraphBuilder *graph_builder)
EsCGraphBuilder *ResolveBuilderImpl(const std::nullptr_t)
EsCGraphBuilder *ResolveBuilder()
EsCGraphBuilder *ResolveBuilder(const T& first, const Ts&... rest)
```
