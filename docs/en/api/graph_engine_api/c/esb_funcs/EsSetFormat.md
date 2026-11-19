# EsSetFormat

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:42:26.391Z pushedAt=2026-11-18T06:41:46.622Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Sets the memory format of the tensor. If not set, the default format is ND.

## Prototype

```c
uint32_t EsSetFormat(EsCTensorHolder *tensor, C_Format format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder pointer. |
| format | Input | Format type. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | uint32_t | Returns 0 on success; otherwise, returns a non-zero value. |

## Constraints

None

## Examples

```c
/ 1. Create a graph builder (EsCGraphBuilder).
EsCGraphBuilder *builder = EsCreateGraphBuilder("graph_name");
// 2. Add the start node.
EsCTensorHolder *data0 = EsCreateInput(builder, 0); // Add the 0th input node, defaulting to the ND format.
EsSetFormat(data0, C_FORMAT_NCHW); // Set the data format of the 0th input node to NCHW.
```
