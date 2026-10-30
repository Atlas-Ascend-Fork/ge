# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:45:22.195Z pushedAt=2026-10-28T07:40:22.635Z nl=2 -->

A utility class for graph modification. It receives a subgraph boundary on the graph and a replacement graph, and uses the replacement graph to replace the nodes within the given boundary on the graph.

## Header File to Include

```c++
#include <ge/fusion/graph_rewriter.h>
```

## Public Member Functions

```c++
static Status Replace(const SubgraphBoundary &subgraph, const Graph &replacement)
static Status Replace(const SubgraphBoundary &subgraph, Graph &&replacement)
```
