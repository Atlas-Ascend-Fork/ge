# InstanceDynamicInputNum

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:32:57.753Z pushedAt=2026-10-28T07:40:22.330Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the number of dynamic input instances.

## Prototype

```c++
CompliantNodeBuilder &InstanceDynamicInputNum(const char_t *ir_name, int32_t num)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ir_name | Input | IR input name. |
| num | Input | Number of instances. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
