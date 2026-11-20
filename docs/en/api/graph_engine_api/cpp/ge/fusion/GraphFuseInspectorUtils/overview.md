# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:27:18.384Z pushedAt=2026-10-28T07:40:22.551Z nl=2 -->

A graph fusion check utility class that provides fusion feasibility determination and result reporting capabilities for custom fusion Passes.

## Header File to Include

```c++
#include <ge/fusion/graph_fuse_inspector_utils.h>
```

## Public Member Functions

```c++
static bool CanFuse(const std::vector<GNode> &nodes_before_fuse, AscendString &failed_reason)
static Status ReportFuse(const std::vector<GNode> &nodes_before_fuse, const std::vector<GNode> &nodes_after_fuse, CustomPassContext &ctx)
```
