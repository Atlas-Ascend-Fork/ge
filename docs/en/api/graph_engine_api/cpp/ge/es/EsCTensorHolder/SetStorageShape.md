# SetStorageShape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:59:53.786Z pushedAt=2026-10-28T07:40:22.445Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the storage shape.

## Prototype

```c++
ge::Status SetStorageShape(const ge::Shape &shape)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Storage shape. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::Status | Operation status.<br><br>  - **0**: Success<br>  - **1**: Failure |

## Constraints

None
