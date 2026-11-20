# SetStorageFormat

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:59:41.027Z pushedAt=2026-10-28T07:40:22.444Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the storage data format.

## Prototype

```c++
ge::Status SetStorageFormat(const ge::Format format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| format | Input | Storage data format. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::Status | Operation status.<br><br>  - **0**: Success<br>  - **1**: Failure |

## Constraints

None
