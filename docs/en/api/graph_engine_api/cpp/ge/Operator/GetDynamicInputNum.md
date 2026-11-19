# GetDynamicInputNum

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:17:31.024Z pushedAt=2026-10-28T07:42:19.665Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the actual number of dynamic inputs of the operator.

## Prototype

> [!NOTE]
> The API whose data type is string will be deprecated in later versions. Use the API whose data type is not string instead.

```c++
int32_t GetDynamicInputNum(const std::string &name) const
int32_t GetDynamicInputNum(const char_t *name) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the dynamic input of the operator. |

## Return Value

The actual number of dynamic inputs.

Returns **-1** if name is invalid or the operator has no dynamic input.

## Constraints

None
