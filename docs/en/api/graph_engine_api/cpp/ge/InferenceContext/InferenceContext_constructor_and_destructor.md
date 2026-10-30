# InferenceContext Constructor and Destructor

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T14:31:14.654Z pushedAt=2026-10-28T07:40:22.797Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

The constructor and destructor of the **InferenceContext** object.

## Prototype

```c++
~InferenceContext() = default
InferenceContext(const InferenceContext &context) = delete
InferenceContext(const InferenceContext &&context) = delete
InferenceContext &operator=(const InferenceContext &context) = delete
InferenceContext &operator=(const InferenceContext &&context) = delete
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| context | Input | Content of **InferenceContext**, used for initialization. |

## Return Value

The **InferenceContext** constructor returns an object of the **InferenceContext** type.

## Constraints

None
