# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:19:53.386Z pushedAt=2026-10-28T07:40:22.516Z nl=2 -->

Detailed information about the persisted external weight.

## Required Header File

```c++
#include <ge/ge_external_weight_desc.h>
```

## Public Member Functions

```c++
~ExternalWeightDesc();
ExternalWeightDesc &operator=(const ExternalWeightDesc &) & = delete;
ExternalWeightDesc(const ExternalWeightDesc &) = delete;
AscendString GetLocation() const;
size_t GetSize() const;
size_t GetOffset() const;
AscendString GetId() const;
```
