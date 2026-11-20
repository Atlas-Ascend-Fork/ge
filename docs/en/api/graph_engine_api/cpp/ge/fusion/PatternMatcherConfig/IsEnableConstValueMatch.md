# IsEnableConstValueMatch

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:36:14.584Z pushedAt=2026-10-28T07:40:22.592Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pattern\_matcher\_config.h\>
- Library file: libge\_compiler.so

## Description

Returns whether the Const value matching capability is enabled.

## Prototype

```c++
bool IsEnableConstValueMatch() const
```

## Parameters

None

## Return Value

Values of the bool type:

- If **true** is returned, the pattern matcher matches the values of the Tensors carried by the const/constant defined in the pattern during matching. A match is considered successful only when the values are equal.
- If **false** is returned, the Const on the node does not participate in matching.

## Constraints

The matching method is binary matching, which is a relatively strict matching method (for floating-point data types). If it does not meet expectations, disable this switch and handle the value validation logic in the Pass yourself.
