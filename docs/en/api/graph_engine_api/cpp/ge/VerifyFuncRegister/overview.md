# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:36:47.816Z pushedAt=2026-10-28T07:42:19.880Z nl=2 -->

API for registering the verifyFunc function of an operator. This API is referenced by other header files and generally does not need to be called directly by operator developers.

## Header File to Include

```c++
#include <graph/operator_factory.h>
```

## Public Member Functions

```c++
VerifyFuncRegister(const std::string &operator_type, const VerifyFunc &verify_func)
VerifyFuncRegister(const char_t *const operator_type, const VerifyFunc &verify_func)
~VerifyFuncRegister() = default
```
