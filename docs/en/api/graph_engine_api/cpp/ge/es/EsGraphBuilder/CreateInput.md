# CreateInput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:02:16.676Z pushedAt=2026-10-28T07:40:22.452Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates an input node.

## Prototype

- Creates a graph input node.

    ```c++
    EsTensorHolder CreateInput(int64_t index,const char *name,const char *type)
    ```

- Creates a default input node, counting from 0, with the node named **input\_\{index\}**

    ```c++
    EsTensorHolder CreateInput(int64_t index)
    ```

- Creates an input node with a specified name.

    ```c++
    EsTensorHolder CreateInput(int64_t index, const char *name)
    ```

- Creates an input node with complete information.

    ```c++
    EsTensorHolder CreateInput(int64_t index, const char *name, ge::DataType data_type, ge::Format format, const std::vector<int64_t> &shape)
    ```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Index of the input node, starting from 0. |
| name | Input | Name of the input node. If it is nullptr, the default name input_{index} is used. |
| type | Input | Type string of the input node. If it is nullptr, the default is Data. |
| format | Input | Tensor format. |
| data_type | Input | Data type. |
| shape | Input | Tensor shape vector. If it is empty, a scalar is created. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsTensorHolder | Returns the created input tensor holder. On failure, returns an invalid **EsTensorHolder**. |

## Constraints

None

## Examples

- Create a graph input node.

```c++
    EsGraphBuilder builder("graph_name");
    auto t1 = builder.CreateInput(0, "input0", "Data");
    ```

-   Create a default input node, start counting from 0, and name the node **input\_\{index\}**

    ```c++
    EsGraphBuilder builder("graph_name");
    auto tensor = builder.CreateInput(1);
    ```

-   Create an input node with a specified name.

    ```c++
    EsGraphBuilder builder("graph_name");
    auto tensor = builder.CreateInput(2, "input2");
    ```

-   Create an input node with complete information.

    ```c++
    EsGraphBuilder builder("graph_name");
    auto tensor = builder.CreateInput(3, "input3", ge::DT_INT32, ge::FORMAT_NCHW, {2, 2});
    ```
