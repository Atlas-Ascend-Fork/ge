# EsSetInt64AttrForTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:43:57.347Z pushedAt=2026-11-18T06:42:00.638Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Sets the int64 attribute of a tensor.

## Prototype

```c
uint32_t EsSetInt64AttrForTensor(EsCTensorHolder *tensor, const char *attr_name, int64_t value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder pointer. |
| attr_name | Input | Attribute name. |
| value | Input | Attribute value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | uint32_t | Returns 0 on success; otherwise, returns a non-zero value. |

## Constraints

None

## Examples

```c
EsCGraphBuilder *graph = EsCreateGraphBuilder("test_graph");
auto *tensor = EsCreateGraphInput(graph, 0);
uint32_t result = EsSetInt64AttrForTensor(tensor, "int64_attr", 10);
```
