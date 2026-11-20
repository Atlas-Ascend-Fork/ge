# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:27:09.467Z pushedAt=2026-10-28T07:40:22.779Z nl=2 -->

Used to store the cached data of a graph.

## Header File to Include

```c++
#include <graph/graph_buffer.h>
```

## Public Member Functions

```c++
GraphBuffer()
GraphBuffer(const GraphBuffer &) = delete
GraphBuffer &operator=(const GraphBuffer &) = delete
~GraphBuffer()
const std::uint8_t *GetData() const
std::size_t GetSize() const
```
