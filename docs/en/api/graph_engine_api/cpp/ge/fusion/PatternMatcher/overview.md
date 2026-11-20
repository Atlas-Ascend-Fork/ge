# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:34:29.037Z pushedAt=2026-10-28T07:40:22.582Z nl=2 -->

A utility class for subgraph matching. It receives a **Pattern** definition and the **Graph** to be matched, matches the **Graph** against the **Pattern**, and provides the **MatchNext** API to return the matching results one by one.

## Required Header Files

```c++
#include <ge/fusion/pattern_matcher.h>
```

## Public Member Functions

```c++
explicit PatternMatcher(std::unique_ptr<Pattern> pattern, const GraphPtr &target_graph)
[[nodiscard]] std::unique_ptr<MatchResult> MatchNext()
```
