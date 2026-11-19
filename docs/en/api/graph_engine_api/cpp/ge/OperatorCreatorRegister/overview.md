# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:34:34.141Z pushedAt=2026-10-28T07:42:19.720Z nl=2 -->

Operator registration API that registers an operator prototype. This API is referenced by other header files and is generally not called directly by you.

## Header File to Include

```c++
#include <graph/operator_factory.h>
```

## Public Member Functions

```c++
OperatorCreatorRegister(const std::string &operator_type, OpCreator const &op_creator)
OperatorCreatorRegister(const char_t *const operator_type, OpCreatorV2 const &op_creator)
~OperatorCreatorRegister() = default
```
