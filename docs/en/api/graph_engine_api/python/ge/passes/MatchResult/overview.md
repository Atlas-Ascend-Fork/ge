# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:55:20.683Z pushedAt=2026-10-16T10:53:07.618Z -->

The result of a pattern match, which is a borrowed C++-side **MatchResult** whose lifecycle is held by the engine/bridging layer. It is used as an input parameter in **PatternFusionPass.meet\_requirements** and **replacement**.

**Constraints**

- Do not retain **MatchResult** beyond the current **meet\_requirements**/**replacement** call.
- After the bridging layer calls **\_invalidate\(\)** in **finally**, subsequent method calls may fail.
- **get\_captured\_tensor\(i\)**: **i** must be consistent with the order of **capture\_tensor**; otherwise, the C++ layer fails and throws an exception.
- **\_invalidate\(\)** is intended only for the GE bridging layer to call; do not call it in pass code.
