# SetDataType

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T13:13:41.262Z pushedAt=2026-10-28T07:40:22.493Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the data type of the **TensorHolder**.

## Prototype

```c++
EsTensorHolder &SetDataType(const ge::DataType data_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data_type | Input | Data type. For details about data types, see [DataType](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsTensorHolder & | Returns a reference to the current object, supporting chained calls. |

## Constraints

None

## Examples

```c++
// 1. Create a graph builder (EsGraphBuilder).
EsGraphBuilder builder("graph_name");
// 2. Add an input node.
EsTensorHolder data0 = builder.CreateInput(0).SetDataType(DT_INT32); // Set the data type of the output tensor of the input node.
```
