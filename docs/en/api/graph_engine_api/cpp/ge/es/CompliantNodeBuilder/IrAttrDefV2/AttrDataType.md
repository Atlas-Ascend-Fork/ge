# AttrDataType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:41:40.450Z pushedAt=2026-10-28T07:40:22.368Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the data type of the attribute.

## Prototype

```c++
IrAttrDefV2 &AttrDataType(const char_t *attr_data_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_data_type | Input | Data type string of the attribute value. Typical values include:<br><br>  - "int": integer type<br>  - "float": floating-point type<br>  - "string": string type<br>  - "bool": boolean type<br>  - "list_int": integer list<br>  - "list_float": floating-point list |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | IrAttrDefV2 & | Self-reference of the IR attribute definition class, supporting chained calls. |

## Constraints

None
