# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:36:12.617Z pushedAt=2026-10-28T07:42:19.729Z nl=2 -->

An internal associated API that is referenced by other header files and is generally not called directly by operator developers.

## Header File to Include

```c++
#include <graph/operator_factory.h>
```

## Public Member Functions

```c++
static Operator CreateOperator(const std::string &operator_name, const std::string &operator_type)
static Operator CreateOperator(const char_t *const operator_name, const char_t *const operator_type)
static graphStatus GetOpsTypeList(std::vector<std::string> &all_ops)
static graphStatus GetOpsTypeList(std::vector<AscendString> &all_ops)
static bool IsExistOp(const std::string &operator_type)
static bool IsExistOp(const char_t *const operator_type)
```
