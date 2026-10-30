# SetDataType

<!-- md-trans-meta sourceCommit=ba12e592e8f84bc59709464bd3ad6ca010498d8b translatedAt=2026-10-16T12:57:21.363Z pushedAt=2026-10-28T07:40:22.436Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the data type.

## Prototype

```c++
ge::Status SetDataType(const ge::DataType data_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **data_type** | Input | Data type. For details, see [DataType](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/ge_namespace/DataType.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::Status | Operation status.<br><br>  - **0**: Success<br>  - **1**: Failure |

## Constraints

None
