# CreateOperator

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:34:51.584Z pushedAt=2026-10-28T07:42:19.725Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/operator\_factory.h\>
- Library file: libgraph.so

## Description

Obtains an operator object instance based on the operator name and operator type.

## Prototype

> [!NOTE]
> APIs with the string data type will be deprecated in later versions. You are advised to use APIs with a non-string data type.

```c++
static Operator CreateOperator(const std::string &operator_name, const std::string &operator_type)
static Operator CreateOperator(const char_t *const operator_name, const char_t *const operator_type)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| operator_name | Input | Operator name. |
| operator_type | Input | Operator type. |

## Return Value

Operator object instance.

## Constraints

None
