# GetValue

<!-- md-trans-meta sourceCommit=8ed641509c09b657d721fdefadfbf6dee35dac94 translatedAt=2026-10-16T12:16:04.483Z pushedAt=2026-10-28T07:40:22.262Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/attr\_value.h\>
- Library file: libgraph\_base.so

## Description

Obtains the value in the attribute key-value pair and converts the value from type T to type DT.

- Supports converting the INT type to the int64\_t type.
- Supports converting the FLOAT type to the float type.
- Supports converting the STR type to the std::string type.

## Prototype

> [!NOTE]
> The API whose data type is string will be deprecated in later versions. It is recommended to use the API whose data type is not string.

```c++
template<typename T, typename DT>
graphStatus GetValue(DT &val) const
graphStatus GetValue(AscendString &val)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| val | Output | Parameter of the DT type. |

## Return Value

**graphStatus** type: Returns **GRAPH\_SUCCESS** if the data type conversion succeeds; otherwise, returns **GRAPH\_FAILED**.

## Constraints

None
