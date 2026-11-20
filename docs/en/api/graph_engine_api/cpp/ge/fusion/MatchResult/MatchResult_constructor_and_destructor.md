# MatchResult Constructor and Destructor

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T13:29:16.436Z pushedAt=2026-10-28T07:40:22.560Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/match\_result.h\>
- Library file: libge\_compiler.so

## Description

Constructor and destructor of **MatchResult**.

Defines the matching result, with the input parameter being **Pattern**, that is, the matching definition. In **MatchResult**, a **Matched Node** is a node matched based on a **Pattern Node**, and corresponds one-to-one with the **Pattern Node**.

## Prototype

```c++
explicit MatchResult(const Pattern *pattern);
MatchResult(const MatchResult &other) noexcept;
MatchResult(MatchResult &&other) noexcept;
MatchResult &operator=(const MatchResult &other) noexcept;
MatchResult &operator=(MatchResult &&other) noexcept ;
~MatchResult();
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pattern | Input | **Pattern** definition. |

## Return Value

None

## Constraints

None
