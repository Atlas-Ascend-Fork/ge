# GetPlatformInfos

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:13:26.871Z pushedAt=2026-10-28T08:50:45.488Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/op\_compile\_context.h\>
- Library file: liblowering.so

## Description

Obtains the compile-time platform hardware information and optional supplementary information.

## Prototype

```c++
ge::graphStatus GetPlatformInfos(fe::PlatFormInfos &platform_info, fe::OptionalInfos& optional_infos) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| platform_info | Output | Used to store platform hardware information, including the SoC version, chip specifications, and so on. |
| optional_infos | Output | Used to store optional supplementary information. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Constraints

None
