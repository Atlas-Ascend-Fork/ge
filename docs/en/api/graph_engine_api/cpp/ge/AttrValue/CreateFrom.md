# CreateFrom

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T12:15:13.354Z pushedAt=2026-10-28T07:40:22.259Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/attr\_value.h\>
- Library file: libgraph\_base.so

## Description

Converts the passed-in parameter of the DT type (supporting int64\_t, float, and std::string types) into a parameter of the corresponding T type (supporting INT, FLOAT, and STR types).

- Supports type conversion from int64\_t to INT.
- Supports type conversion from float to FLOAT.
- Supports type conversion from std::string to STR.

## Prototype

```c++
template<typename T, typename DT>
static T CreateFrom(DT &&val)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| val | Input | Parameter of the DT type. |

## Return Value

Returns a parameter of the T type.

## Constraints

None
