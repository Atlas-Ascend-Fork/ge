# GetInferenceContext

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:19:43.460Z pushedAt=2026-10-28T07:42:19.672Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the associated information required by the current operator for infershape inference, such as the shape and DataType information of the preceding operator.

## Prototype

```c++
InferenceContextPtr GetInferenceContext() const
```

## Parameters

None

## Return Value

Returns the inference context of the current operator.

InferenceContextPtr is an alias for a pointer to the InferenceContext class:

```c++
using InferenceContextPtr = std::shared_ptr<InferenceContext>;
```

## Constraints

None
