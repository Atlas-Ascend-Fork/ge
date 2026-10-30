# SetFormat

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T12:57:42.489Z pushedAt=2026-10-28T07:40:22.438Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the data format.

## Prototype

```c++
ge::Status SetFormat(const ge::Format format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| format | Input | Data format. For details, see [Format](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::Status | Operation status.<br><br>  - **0**: Success<br>  - **1**: Failure |

## Constraints

None
