# SetFormat

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T13:14:04.804Z pushedAt=2026-10-28T07:40:22.494Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the data format of **TensorHolder**.

## Prototype

```c++
EsTensorHolder &SetFormat(const ge::Format format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| format | Input | Tensor format. For details, see [Format](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsTensorHolder & | Returns a reference to the current object, supporting chained calls. |

## Constraints

None

## Examples

```c++
// 1. Create the graph builder (EsGraphBuilder).
EsGraphBuilder builder("graph_name");
// 2. Add the input node.
EsTensorHolder data0 = builder.CreateInput(0).SetFormat(FORMAT_NCHW); // Set the format type of the output tensor of the input node.
```
