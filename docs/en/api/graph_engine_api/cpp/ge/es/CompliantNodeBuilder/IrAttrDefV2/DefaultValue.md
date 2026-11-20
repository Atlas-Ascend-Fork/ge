# DefaultValue

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:42:28.255Z pushedAt=2026-10-28T07:40:22.373Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the default value of the attribute.

## Prototype

```c++
IrAttrDefV2 &DefaultValue(const AttrValue &attr_default_value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **attr_default_value** | Input | Default value of the attribute, used for optional attributes. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | **IrAttrDefV2 &** | Reference to the IR attribute definition class itself, supporting chained calls. |

## Constraints

None
