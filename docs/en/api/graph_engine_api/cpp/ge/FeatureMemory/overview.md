# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:21:29.948Z pushedAt=2026-10-28T07:40:22.525Z nl=2 -->

Used to store **Feature** memory information.

## Header File to Include

```c++
#include <ge/ge_feature_memory.h>
```

## Public Member Functions

```c++
~FeatureMemory()
FeatureMemory &operator=(const FeatureMemory &) & = delete
FeatureMemory(const FeatureMemory &) = delete
MemoryType GetType() const
size_t GetSize() const
bool IsFixed() const
```
