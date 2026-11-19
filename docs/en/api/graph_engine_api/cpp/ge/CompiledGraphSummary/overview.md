# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:25:23.507Z pushedAt=2026-10-28T07:40:22.296Z nl=2 -->

Summary information of the graph compilation result.

## Header File to Include

```c++
#include <ge/ge_graph_compile_summary.h>
```

## Public Member Functions

```c++
~CompiledGraphSummary()
CompiledGraphSummary &operator=(const CompiledGraphSummary &) & = delete
CompiledGraphSummary(const CompiledGraphSummary &) = delete
bool IsStatic() const
Status GetConstMemorySize(size_t &size) const
Status GetFeatureMemorySize(size_t &size) const
Status GetFixedFeatureMemorySize(size_t &size) const
std::vector<FeatureMemoryPtr> GetAllFeatureMemoryTypeSize() const
Status GetRefreshableFeatureMemorySize(size_t &size) const
Status GetFeatureMemoryBaseRefreshable (bool &v) const
Status GetStreamNum(size_t &num) const
Status GetEventNum(size_t &num) const
Status GetOutputShapes(std::vector<ge::Shape> &shapes) const
Status GetOutputDtypes(std::vector<ge::DataType> &dtypes) const
Status GetIOIndexesWithSameAddr(std::vector<std::pair<uint32_t, uint32_t>> &io_indexes) const
Status GetExternalWeightPaths(std::vector<ExternalWeightDescPtr> &paths) const
Status GetStreamAllocationSummary(std::shared_ptr<StreamAllocationSummary> &stream_allocation) const
```
