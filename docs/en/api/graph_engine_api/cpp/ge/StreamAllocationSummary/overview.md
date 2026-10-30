# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:32:50.344Z pushedAt=2026-10-28T07:42:19.858Z nl=2 -->

Summary information of stream allocation.

## Header File to Include

```c++
#include <ge/ge_graph_compile_summary.h>
```

## Public Member Functions

```c++
const std::map<AscendString, AscendString> &ToStreamGraph() const;
const std::map<AscendString, std::vector<LogicalStreamAllocationInfo>> &GetAllLogicalStreamInfos() const;
```
