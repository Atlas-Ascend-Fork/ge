# EnableConstValueMatch

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:38:07.015Z pushedAt=2026-10-28T07:40:22.599Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pattern\_matcher\_config.h\>
- Library file: libge\_compiler.so

## Description

Configures the constant value matching capability of **PatternMatcher**.

After this function is called, **PatternMatcher** performs strict value comparison on the **Tensor** carried by the const/constant nodes defined in the pattern during graph matching. A match is considered successful only when the constant value in the actual graph is exactly equal to the constant value defined in the pattern (binary match).

## Prototype

```c++
PatternMatcherConfigBuilder &EnableConstValueMatch()
```

## Parameters

None

## Return Value

A reference to **PatternMatcherConfigBuilder**, which allows cascaded calls to member functions.

## Constraints

The matching method is a binary match, which is a relatively strict matching method. If it does not meet expectations, disable this configuration and handle the value validation logic in the pass yourself.
