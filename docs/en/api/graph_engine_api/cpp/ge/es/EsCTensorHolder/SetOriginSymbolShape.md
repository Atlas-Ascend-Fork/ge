# SetOriginSymbolShape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:58:51.323Z pushedAt=2026-10-28T07:40:22.441Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the original symbol shape.

## Prototype

```c++
ge::Status SetOriginSymbolShape(const char *const *shape_str, const int64_t shape_str_num)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape_str | Input | Array of shape strings. |
| shape_str_num | Input | Number of shape strings. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::Status | Operation status.<br><br>  - **0**: Success<br>  - **1**: Failure |

## Constraints

None
