# GeGraphsToEsCGraphs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:08:00.321Z pushedAt=2026-10-28T07:40:22.470Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Converts a dynamic subgraph into an anonymous pointer type subgraph container.

## Prototype

```c++
inline std::vector<EsCGraph *> GeGraphsToEsCGraphs(std::vector<std::unique_ptr<ge::Graph>> graphs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graphs | Input | Container of dynamic subgraph input parameters of the control operator. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::vector<EsCGraph *> | Subgraph container of anonymous pointer type. |

## Constraints

None
