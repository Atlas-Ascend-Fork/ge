# Build

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:37:36.685Z pushedAt=2026-10-28T07:40:22.597Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pattern\_matcher\_config.h\>
- Library file: libge\_compiler.so

## Description

API for constructing a **PatternMatcherConfig** object.

## Prototype

```c++
std::unique_ptr<PatternMatcherConfig> Build() const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| std::unique_ptr\<PatternMatcherConfig> | Output | Matching rule. |

## Return Value

None

## Constraints

None
