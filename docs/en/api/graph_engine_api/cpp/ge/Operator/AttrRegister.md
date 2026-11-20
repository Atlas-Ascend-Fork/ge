# AttrRegister

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:14:12.455Z pushedAt=2026-10-28T07:42:19.655Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Generic attribute registration API.

## Prototype

```c++
void AttrRegister(const char_t *name, const AttrValue &attr_value)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |
| attr_value | Input | Attribute value. |

## Return Value

**graphStatus** type: Returns **GRAPH\_SUCCESS** on success; otherwise, returns **GRAPH\_FAILED**.

## Constraints

None
