# VerifyFuncRegister Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:37:31.615Z pushedAt=2026-10-28T07:42:19.881Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_factory.h\>

## Description

**VerifyFuncRegister** constructor and destructor.

## Prototype

Note: APIs whose data type is string will be deprecated in later versions. Use APIs whose data type is not string instead.

```c++
VerifyFuncRegister(const std::string &operator_type, const VerifyFunc &verify_func)
VerifyFuncRegister(const char_t *const operator_type, const VerifyFunc &verify_func)
~VerifyFuncRegister() = default
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| operator_type | Input | Operator type. |
| verify_func | Input | Operator verify function. |

## Return Value

The **VerifyFuncRegister** constructor returns an object of the **VerifyFuncRegister** type.

## Constraints

API for registering the operator **verifyFunc** function. This API is referenced by other header files and generally is not directly called by operator developers.
