# KernelLaunchInfo Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:39:37.060Z pushedAt=2026-10-28T07:40:22.835Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

**KernelLaunchInfo** constructor and destructor.

## Prototype

```c++
~KernelLaunchInfo()
KernelLaunchInfo(const KernelLaunchInfo &other)
KernelLaunchInfo(KernelLaunchInfo &&other) noexcept
KernelLaunchInfo &operator=(const KernelLaunchInfo &other)
KernelLaunchInfo &operator=(KernelLaunchInfo &&other) noexcept
```

## Parameters

None

## Return Value

None

## Constraints

None
