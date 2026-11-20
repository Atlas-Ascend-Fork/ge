# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:29:09.143Z pushedAt=2026-10-28T07:40:22.316Z nl=2 -->

Base class for implementing a Pass for one-to-many scenario fusion. This class provides the capability to find nodes in the graph by node type and to modify the graph based on the **Replacement** structure provided by subclasses.

## Header File to Include

```c++
#include <ge/fusion/pass/decompose_pass.h>
```

## Public Member Functions

```c++
explicit DecomposePass(const std::vector<AscendString> &op_types)
Status Run(GraphPtr &graph, CustomPassContext &pass_context) override
```
