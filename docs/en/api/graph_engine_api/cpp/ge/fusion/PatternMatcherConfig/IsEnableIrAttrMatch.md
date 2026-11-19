# IsEnableIrAttrMatch

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:36:26.879Z pushedAt=2026-10-28T07:40:22.593Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pattern\_matcher\_config.h\>
- Library file: libge\_compiler.so

## Description

Returns whether IR attribute and its value matching is enabled.

## Prototype

```c++
bool IsEnableIrAttrMatch() const
```

## Parameters

None

## Return Value

Values of the bool type:

- If **true** is returned, the pattern matcher matches the values of IR attributes carried on nodes in the pattern during pattern matching.
- If **false** is returned, IR attributes on nodes do not participate in matching.

## Constraints

The matching method is binary matching, which is a relatively strict matching method (for floating-point data types). If it does not meet expectations, disable this switch and handle the value validation logic in the pass.
