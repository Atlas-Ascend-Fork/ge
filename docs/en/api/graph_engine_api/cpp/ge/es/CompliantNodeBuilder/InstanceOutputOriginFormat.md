# InstanceOutputOriginFormat

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T12:34:27.488Z pushedAt=2026-10-28T07:40:22.338Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the original output format.

## Prototype

```c++
CompliantNodeBuilder &InstanceOutputOriginFormat(const char_t *name, ge::Format format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Output name. |
| format | Input | Original format. For details, see [Format](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
