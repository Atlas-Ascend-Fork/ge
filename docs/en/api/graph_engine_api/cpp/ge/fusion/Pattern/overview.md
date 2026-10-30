# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:32:34.815Z pushedAt=2026-10-28T07:40:22.573Z nl=2 -->

A template used to define the graph structure to be matched. The template receives a **Graph**, which is used to describe the graph structure to be matched.

## Header File to Include

```c++
#include <ge/fusion/pattern.h>
```

## Public Member Functions

```c++
explicit Pattern(Graph &&pattern_graph)
const Graph &GetGraph() const
Pattern &CaptureTensor(const NodeIo &node_output)
Status GetCapturedTensors(std::vector<NodeIo> &node_outputs) const
```
