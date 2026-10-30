# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:30:02.652Z pushedAt=2026-10-28T07:40:22.561Z nl=2 -->

A result matched on the graph through [PatternMatcher](../PatternMatcher/PatternMatcher.md), which contains the matched node.

## Header File to Include

```c++
#include <ge/fusion/match_result.h>
```

## Public Member Functions

```c++
MatchResult()
explicit MatchResult(const Pattern *pattern)
MatchResult(const MatchResult &other) noexcept
MatchResult(MatchResult &&other) noexcept
MatchResult &operator=(const MatchResult &other) noexcept
MatchResult &operator=(MatchResult &&other) noexcept
Status AppendNodeMatchPair(const NodeIo &pattern_node_out,const NodeIo &matched_node_out)
Status GetMatchedNode(const GNode &pattern_node, GNode &matched_node) const
[[nodiscard]] std::vector<GNode> GetMatchedNodes() const
Status GetCapturedTensor(size_t capture_idx, NodeIo &node_output) const
[[nodiscard]] std::unique_ptr<SubgraphBoundary> ToSubgraphBoundary() const
[[nodiscard]] AscendString ToAscendString() const
```
