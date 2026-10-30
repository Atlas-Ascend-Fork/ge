# SetInferenceContext

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:27:34.544Z pushedAt=2026-10-28T07:42:19.698Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Passes the associated information required for infershape inference to the current operator, such as the shape and DataType information of the preceding operator.

## Prototype

```c++
void SetInferenceContext(const InferenceContextPtr &inference_context)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| inference_context | Input | Inference context of the current operator.<br>InferenceContextPtr is an alias of a pointer to the InferenceContext class:<br>using InferenceContextPtr = std::shared_ptr\<InferenceContext>; |

## Return Value

None

## Constraints

None
