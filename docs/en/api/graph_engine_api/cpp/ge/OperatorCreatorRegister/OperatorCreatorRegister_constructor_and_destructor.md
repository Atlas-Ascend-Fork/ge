# OperatorCreatorRegister Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:33:41.718Z pushedAt=2026-10-28T07:42:19.722Z nl=2 -->

## Header File

\#include <graph/operator\_factory.h\>

## Description

Constructor and destructor of OperatorCreatorRegister.

## Prototype

> [!NOTE]
> APIs with the string data type will be deprecated in later versions. Use APIs with a non-string data type instead.

```c++
OperatorCreatorRegister(const std::string &operator_type, OpCreator const &op_creator)
OperatorCreatorRegister(const char_t *const operator_type, OpCreatorV2 const &op_creator)
~OperatorCreatorRegister() = default
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| operator_type | Input | Operator type. |
| op_creator | Input | Operator constructor. |

## Return Value

The OperatorCreatorRegister constructor returns an object of the OperatorCreatorRegister type.

## Constraints

Operator registration API. It registers an operator prototype. This API is referenced by other header files and generally does not need to be called directly by you.
