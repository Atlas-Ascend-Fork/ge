# IsGraphNeedRebuild

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:18:15.803Z pushedAt=2026-10-28T07:42:19.810Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Whether the Graph needs to be recompiled.

## Prototype

```c++
bool IsGraphNeedRebuild(uint32_t graph_id)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the Graph. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | bool | - **False**: The graph can be called normally by [RunGraphAsync](RunGraphAsync.md).<br>  - **True**: [RemoveGraph](RemoveGraph.md) must be called first, and then [AddGraph](AddGraph.md) and [BuildGraph](BuildGraph.md) must be called again. |

## Constraints

None
