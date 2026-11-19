# EsSetDataType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:42:14.864Z pushedAt=2026-11-18T06:41:43.225Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Function Description

Sets the data type of the Tensor. If not set, the default is float.

## Prototype

```c
uint32_t EsSetDataType(EsCTensorHolder *tensor, C_DataType data_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder pointer. |
| data_type | Input | Data type. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | uint32_t | 0 on success; otherwise, a failure value. |

## Constraints

None

## Examples

```c
/ 1. Create the graph builder (EsCGraphBuilder).
EsCGraphBuilder *builder = EsCreateGraphBuilder("graph_name");
// 2. Add the start node.
EsCTensorHolder *data0 = EsCreateInput(builder, 0); // Add the 0th input node, defaulting to the FLOAT type.
EsSetDataType(data0, C_DT_INT32); // Set the data type of the 0th input node.
```
