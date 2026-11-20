# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:33:41.900Z pushedAt=2026-10-28T07:40:22.577Z nl=2 -->

A fusion pass base class based on Pattern matching. Subclasses can define the pattern and replacement by implementing virtual methods. The parent class performs the matching and replacement.

## Header File to Include

```c++
#include <ge/fusion/pass/pattern_fusion_pass.h>
```

## Public Member Functions

```c++
PatternFusionPass()
explicit PatternFusionPass(std::unique_ptr<PatternMatcherConfig> match_config)
Status Run(GraphPtr &graph, CustomPassContext &pass_context) override
```
