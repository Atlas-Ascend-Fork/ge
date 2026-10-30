# EsTensorLike Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:15:21.701Z pushedAt=2026-10-28T07:40:22.499Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_like.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

**EsTensorLike** constructor and destructor.

## Prototype

```c++
EsTensorLike(const EsTensorHolder &tensor)
EsTensorLike(const std::nullptr_t)
EsTensorLike(const int64_t value)
EsTensorLike(const float value)
EsTensorLike(const std::vector<int64_t> &values)
EsTensorLike(const std::vector<float> &values)
~EsTensorLike()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | **EsTensorHolder** object. |
| nullptr_t | Input | Null pointer. |
| value | Input | **int64_t** or **float** scalar. |
| values | Input | **int64_t** or **float** vector. |

## Return Value

None

## Constraints

None

## Examples

```c++
EsTensorHolder tensor = builder->CreateScalar(int64_t(1));
EsTensorLike tensor_like(tensor);
```
