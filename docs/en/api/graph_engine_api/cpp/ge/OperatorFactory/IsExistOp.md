# IsExistOp

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:35:19.546Z pushedAt=2026-10-28T07:42:19.732Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/operator\_factory.h\>
- Library file: libgraph.so

## Description

Queries whether the specified operator type is supported.

## Prototype

> [!NOTE]
>The API whose data type is string will be deprecated in later versions. You are advised to use the API whose data type is not string.

```c++
static bool IsExistOp(const std::string &operator_type)
static bool IsExistOp(const char_t *const operator_type)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| operator_type | Input | Operator type. |

## Return Value

- true: The operator exists.
- false: The operator does not exist.

## Constraints

None
