# PatternMatcher Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:35:17.960Z pushedAt=2026-10-28T07:40:22.588Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pattern\_matcher.h\>
- Library file: libge\_compiler.so

## Description

Constructs a **PatternMatcher** from a **Pattern** and a target **Graph**.

## Prototype

```c++
explicit PatternMatcher(std::unique_ptr<Pattern> pattern, const GraphPtr &target_graph)
~PatternMatcher()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pattern | Input | **Pattern** definition. After the **PatternMatcher** is constructed, its lifecycle is managed by the Matcher. |
| target_graph | Input | **Graph** to be matched. |

## Return Value

None

## Constraints

None
