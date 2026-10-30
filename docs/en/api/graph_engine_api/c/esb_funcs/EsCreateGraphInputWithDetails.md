# EsCreateGraphInputWithDetails

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:18:13.629Z pushedAt=2026-10-28T13:21:21.661Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

Creates an input on the graph.

## Prototype

```c
EsCTensorHolder *EsCreateGraphInputWithDetails(EsCGraphBuilder *graph, int64_t index, const char *name, const char *type, C_DataType data_type, C_Format format, const int64_t *shape, int64_t dim_num);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph builder pointer. |
| index | Input | Index of the input in the graph, starting from 0. |
| name | Input | Name of the input. If empty, it defaults to input_{index}. |
| type | Input | Type of the input. If empty, it defaults to Data. |
| data_type | Input | Data type. |
| format | Input | Format type. |
| shape | Input | Shape information. If empty, the input is a scalar. |
| dim_num | Input | Number of dimensions. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCTensorHolder * | The returned tensor resource is managed by the graph builder. |

## Constraints

None

## Examples

```c
#include "c_types.h" // Include the enum type definitions of c.
// 1. Create a graph builder (EsCGraphBuilder).
EsCGraphBuilder *builder = EsCreateGraphBuilder("graph_name");
// 2. Prepare the shape (dimension information).
// For example: Batch=1, Channel=3, Height=224, Width=224.
int64_t input_shape[] = {1, 3, 224, 224};

// Calculate the number of dimensions (dim_num).
int64_t dim_num = sizeof(input_shape) / sizeof(input_shape[0]); // The result is 4.

// 3. Prepare other parameters.
int64_t input_index = 0;           // The 0th input.
const char *input_name = "data";   // Input node name.
const char *node_type = "Data";   // Node type of the input.
C_DataType dtype = C_DT_FLOAT;   // Data type is float.
C_Format fmt = C_FORMAT_NCHW;         // Data layout is NCHW.

// 4. Call the function.
EsCTensorHolder *input_tensor = EsCreateGraphInputWithDetails(
    builder,      // graph: Graph builder pointer.
    input_index,  // index: Input index.
    input_name,   // name: Input name.
    node_type,    // type: Node type string.
    dtype,        // data_type: Data type enumeration.
    fmt,          // format: Format enumeration.
    input_shape,  // shape: pointer to the dimension array
    dim_num       // dim_num: length of the dimension array
);
```
