# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:59:19.281Z pushedAt=2026-10-28T07:40:22.677Z nl=2 -->

Utility classes related to Graph Engine (GE) graph modification, such as shape inference, checking whether a node is supported, and some graph optimization utility methods.

## Header File to Include

```c++
#include <ge/ge_utils.h>
```

## Public Member Functions

```c++
static Status InferShape(const Graph &graph, const std::vector<Shape> &input_shape)
static Status CheckNodeSupportOnAicore(const GNode &node, bool &is_supported, AscendString &unsupported_reason)
```
