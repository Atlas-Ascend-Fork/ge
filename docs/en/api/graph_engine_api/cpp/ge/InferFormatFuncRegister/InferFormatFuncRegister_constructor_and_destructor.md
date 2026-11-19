# InferFormatFuncRegister Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:34:08.031Z pushedAt=2026-10-28T07:40:22.810Z nl=2 -->

## Header File

\#include <graph/operator\_factory.h\>

## Description

Constructor and destructor of **InferFormatFuncRegister**.

## Prototype

> [!NOTE]
> APIs with the string data type will be deprecated in later versions. It is recommended to use APIs with a non-string data type.

```c++
InferFormatFuncRegister(const std::string &operator_type, const InferFormatFunc &infer_format_func)
InferFormatFuncRegister(const char_t *const operator_type, const InferFormatFunc &infer_format_func)
~InferFormatFuncRegister() = default
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| operator_type | Input | Operator type. |
| infer_format_func | Input | Operator **InferFormat** function. |

## Return Value

The **InferFormatFuncRegister** constructor returns an object of the **InferFormatFuncRegister** type.

## Constraints

This API registers the operator InferFormat function. It is referenced by other header files and is generally not directly called by operator developers.
