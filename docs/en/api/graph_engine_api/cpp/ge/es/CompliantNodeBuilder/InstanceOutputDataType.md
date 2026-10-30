# InstanceOutputDataType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:33:44.622Z pushedAt=2026-10-28T07:40:22.335Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the output data type.

## Prototype

```c++
CompliantNodeBuilder &InstanceOutputDataType(const char_t *name, ge::DataType data_type);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **name** | Input | Output name. |
| **data_type** | Input | Data type. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
