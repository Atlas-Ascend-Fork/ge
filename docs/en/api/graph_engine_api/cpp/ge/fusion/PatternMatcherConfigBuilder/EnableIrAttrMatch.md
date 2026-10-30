# EnableIrAttrMatch

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:38:07.675Z pushedAt=2026-10-28T07:40:22.600Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pattern\_matcher\_config.h\>
- Library file: libge\_compiler.so

## Description

Used to configure the IR attribute and value matching capability of **PatternMatcher**.

After this function is called, **PatternMatcher** strictly compares the IR (Intermediate Representation) attributes carried on the nodes in the pattern during graph matching, including the number of attribute names, the list of attribute names, and the matching of attribute values.

## Prototype

```c++
PatternMatcherConfigBuilder &EnableIrAttrMatch()
```

## Parameters

None

## Return Value

A **PatternMatcherConfigBuilder** & reference that supports cascaded calls to member functions.

## Constraints

The matching method is binary matching, which is a relatively strict matching method. If it does not meet expectations, disable this switch and handle the value validation logic in the **Pass** yourself.
