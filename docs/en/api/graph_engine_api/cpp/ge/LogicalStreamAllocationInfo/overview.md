# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:45:45.620Z pushedAt=2026-10-28T07:40:22.858Z nl=2 -->

Logical stream allocation information.

## Required Header File

```c++
#include <ge/ge_graph_compile_summary.h>
```

## Public Member Functions

```c++
AscendString ToStringInfo() const
int64_t GetLogicalStreamId() const
AscendString GetUsrStreamLabel() const
bool IsAssignedByStreamPass() const
std::vector<int64_t> GetAttachedStreamIds() const
size_t GetPhysicalStreamNum() const
size_t GetHcclFollowedStreamNum() const
const std::vector<GNode> &GetAllNodes() const
```
