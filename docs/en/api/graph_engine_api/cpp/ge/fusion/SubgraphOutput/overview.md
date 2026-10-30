# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:44:23.128Z pushedAt=2026-10-28T07:40:22.627Z nl=2 -->

Describes an output **Tensor** of a **Subgraph**.

## Header File to Include

```c++
#include <ge/fusion/subgraph_boundary.h>
```

## Public Member Functions

```c++
SubgraphOutput()
explicit SubgraphOutput(const NodeIo &node_output)
SubgraphOutput(const SubgraphOutput &other) noexcept
SubgraphOutput &operator=(SubgraphOutput &&other) noexcept
SubgraphOutput &operator=(const SubgraphOutput &other) noexcept
Status SetOutput(const NodeIo &node_output)
Status GetOutput(NodeIo &node_output) const
```
