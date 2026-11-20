# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:36:23.671Z pushedAt=2026-10-28T07:40:22.590Z nl=2 -->

A class used to configure the behavior of **PatternMatcher**.

Use [PatternMatcherConfigBuilder](../PatternMatcherConfigBuilder/PatternMatcherConfigBuilder.md) to construct **PatternMatcherConfig**.

## Header File to Include

```c++
#include <ge/fusion/pattern_matcher_config.h>
```

## Public Member Functions

```c++
PatternMatcherConfig()
~PatternMatcherConfig()
PatternMatcherConfig(const PatternMatcherConfig &other)
PatternMatcherConfig &operator=(const PatternMatcherConfig &other)
PatternMatcherConfig(PatternMatcherConfig &&other) noexcept
PatternMatcherConfig &operator=(PatternMatcherConfig &&other) noexcept
bool IsEnableConstValueMatch() const
bool IsEnableIrAttrMatch() const
```
