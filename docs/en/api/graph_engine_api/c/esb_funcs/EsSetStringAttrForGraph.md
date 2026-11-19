# EsSetStringAttrForGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:45:10.179Z pushedAt=2026-11-18T06:42:15.078Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Sets the string-type attribute of the graph.

## Prototype

```c
uint32_t EsSetStringAttrForGraph(EsCGraphBuilder *graph, const char *attr_name, const char *value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |
| attr_name | Input | Attribute name. |
| value | Input | Attribute value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | uint32_t | Returns 0 on success; otherwise, returns a non-zero value. |

## Constraints

None
