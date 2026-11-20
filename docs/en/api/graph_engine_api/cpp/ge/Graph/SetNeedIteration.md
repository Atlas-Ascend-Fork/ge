# SetNeedIteration

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:24:17.788Z pushedAt=2026-10-28T07:40:22.770Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Marks whether the **Graph** needs to be executed iteratively.

## Prototype

```c++
void SetNeedIteration(bool need_iteration)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| need_iteration | Input | Whether the graph needs to be executed iteratively. Values:<br><br>  - **true**: Execute iteratively.<br>  - **false**: Do not execute iteratively. |

## Return Value

None

## Constraints

It must be used together with npu\_runconfig/iterations\_per\_loop, npu\_runconfig/loop\_cond, npu\_runconfig/one, npu\_runconfig/zero, and so on. The user needs to first construct variable operator nodes named npu\_runconfig/iterations\_per\_loop, npu\_runconfig/loop\_cond, npu\_runconfig/one, and npu\_runconfig/zero.
