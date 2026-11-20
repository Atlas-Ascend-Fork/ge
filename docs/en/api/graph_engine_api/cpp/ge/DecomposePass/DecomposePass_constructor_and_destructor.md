# DecomposePass Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:28:17.415Z pushedAt=2026-10-28T07:40:22.313Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pass/decompose\_pass.h\>
- Library file: libge\_compiler

## Description

**DecomposePass** constructor and destructor.

## Prototype

```c++
explicit DecomposePass(const std::vector<AscendString> &op_types)
virtual ~DecomposePass() = default
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op_types | Input | Node types to be matched. Multiple types are supported. |

## Return Value

None

## Constraints

None
