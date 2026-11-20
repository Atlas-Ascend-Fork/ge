# FindOpByName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:15:59.184Z pushedAt=2026-10-28T07:40:22.738Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Obtains the **op** object cached in the **Graph** based on the operator name.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. It is recommended to use the API with a non-string data type.

```c++
graphStatus FindOpByName(const std::string &name, ge::Operator &op) const
graphStatus FindOpByName(const char_t *name, ge::Operator &op) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the operator to be obtained. |
| op | Output | **op** object required by the user. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **SUCCESS**: The operator instance is obtained successfully.<br>**FAILED**: No **op** object is registered in the Graph under this name. |

## Constraints

This API is optional and is used together with [AddOp](AddOp.md).
