# GetDynamicOutputNum

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:18:26.359Z pushedAt=2026-10-28T07:42:19.666Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the actual number of dynamic Outputs of the operator.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. Use the API with a non-string data type instead.

```c++
int32_t GetDynamicOutputNum(const std::string &name) const
int32_t GetDynamicOutputNum(const char_t *name) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the dynamic Output of the operator. |

## Return Value

The actual number of dynamic Outputs.

Returns 0 if name is invalid or the operator has no dynamic Output.

## Constraints

None
