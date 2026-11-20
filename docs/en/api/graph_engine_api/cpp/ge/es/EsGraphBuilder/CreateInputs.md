# CreateInputs

<!-- md-trans-meta sourceCommit=348a841c78ec999f62926be0bba26d6edb93fdf0 translatedAt=2026-11-17T02:58:42.782Z pushedAt=2026-11-18T02:41:08.919Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates a specified number of input nodes.

## Prototype

- Creates a specified number of input nodes and returns an array, suitable for structured binding at compile time.

    ```c++
    template <size_t inputs_num>
    std::array<EsTensorHolder, inputs_num> CreateInputs(size_t start_index = 0)
    ```

- Creates a specified number of input nodes and returns a container, suitable for dynamic binding at runtime.

    ```
    std::vector<EsTensorHolder> CreateInputs(size_t inputs_num, size_t start_index = 0)
    ```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| inputs_num | Input | Number of input nodes. |
| start_index | Input | Starting index of the input nodes. If it is not 0, it indicates that other input nodes have been created before, and the indexes of all input nodes should increase consecutively starting from 0. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::array<EsTensorHolder, inputs_num> | Returns the array of created input tensor holders. |
| - | std::vector\<EsTensorHolder> | Returns the container of created input tensor holders. |

## Constraints

None

## Examples

- Create a specified number of input nodes and return an array, suitable for structured binding at compile time.

    ```c++
    // 1. Create the graph builder (EsGraphBuilder).
    EsGraphBuilder builder("graph_name");
    // 2. Add 2 input nodes and bind them to data0 and data1 using C++17 structured binding.
    EsTensorHolder [data0, data1] = builder.CreateInputs<2>();
    ```

- Create a specified number of input nodes and return a container, suitable for runtime dynamic binding.

    ```
    // 1. Create a graph builder (EsGraphBuilder).
    EsGraphBuilder builder("graph_name");
    // 2. Add 2 input nodes.
    std::vector<EsTensorHolder> inputs = builder.CreateInputs(2);
    ```
