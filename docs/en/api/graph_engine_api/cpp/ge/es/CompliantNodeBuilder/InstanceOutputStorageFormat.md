# InstanceOutputStorageFormat

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T12:35:31.919Z pushedAt=2026-10-28T07:40:22.343Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the output storage format.

## Prototype

```c++
CompliantNodeBuilder &InstanceOutputStorageFormat(const char_t *name, ge::Format format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Output name. |
| format | Input | Storage format. For details, see [Format](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
