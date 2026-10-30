# IsGraphNeedRebuild

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:50:58.447Z pushedAt=2026-10-28T07:40:22.657Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Whether the graph needs to be recompiled.

## Prototype

```c++
bool IsGraphNeedRebuild(uint32_t graph_id)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | bool | - **False**: The graph can be called normally via [RunGraphAsync](./RunGraphAsync.md).<br>  - **True**: [RemoveGraph](./RemoveGraph.md) must be called first, and then [AddGraph](./AddGraph.md) and [CompileGraph](./CompileGraph.md) must be called again. |

## Constraints

None
