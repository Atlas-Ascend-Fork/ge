# AttrName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:41:44.561Z pushedAt=2026-10-28T07:40:22.369Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the attribute name.

## Prototype

```c++
IrAttrDefV2 &AttrName(const char_t *attr_name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_name | Input | Attribute name, which identifies the name of this attribute in the operator. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | IrAttrDefV2 & | Reference to the IR attribute definition class itself, supporting chained calls. |

## Constraints

None
