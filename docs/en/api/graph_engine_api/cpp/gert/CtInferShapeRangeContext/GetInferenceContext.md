# GetInferenceContext

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:03:05.981Z pushedAt=2026-10-28T08:50:45.449Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/ct\_infer\_shape\_range\_context.h\>

## Description

Obtains the InferenceContext pointer.

## Prototype

```c++
ge::InferenceContext *GetInferenceContext() const
```

## Parameters

None

## Return Value

Outputs the InferenceContext pointer.

For the definition of the InferenceContext type, see [InferenceContext](../../ge/InferenceContext/overview.md).

## Constraints

None

## Examples

```c++
ge::graphStatus InferShapeForXXX(CtInferShapeRangeContext *context) {
  const auto &read_inference_context = ct_context->GetInferenceContext();
  // ...
}
```
