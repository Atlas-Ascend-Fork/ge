# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:38:31.148Z pushedAt=2026-10-28T07:40:22.601Z nl=2 -->

A helper class for constructing **PatternMatcherConfig**.

## Header File to Include

```c++
#include <ge/fusion/pattern_matcher_config.h>
```

## Public Member Functions

```c++
 PatternMatcherConfigBuilder()
~PatternMatcherConfigBuilder()
PatternMatcherConfigBuilder &EnableConstValueMatch()
PatternMatcherConfigBuilder &EnableIrAttrMatch()
std::unique_ptr<PatternMatcherConfig> Build() const
```
