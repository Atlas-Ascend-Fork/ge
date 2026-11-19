# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:34:45.304Z pushedAt=2026-10-28T07:40:22.814Z nl=2 -->

**InferFormat** is the operator InferFormat function registration API. This API is referenced by other header files and generally does not need to be called directly by operator developers.

## Header File to Include

```c++
#include <graph/operator_factory.h>
```

## Public Member Functions

```c++
InferFormatFuncRegister(const std::string &operator_type, const InferFormatFunc &infer_format_func)
InferFormatFuncRegister(const char_t *const operator_type, const InferFormatFunc &infer_format_func)
~InferFormatFuncRegister() = default
```
