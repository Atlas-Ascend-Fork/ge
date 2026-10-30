# BuildAndReset

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:00:27.432Z pushedAt=2026-10-28T07:40:22.448Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Builds a computation graph.

## Prototype

- Builds a computation graph.

```c++
    std::unique_ptr<Graph> BuildAndReset() const
    ```

-   Builds a computation graph based on the output tensor list.

    ```
    std::unique_ptr<Graph> BuildAndReset(const std::vector<EsTensorHolder> &outputs)
    ```

## Parameters


| Parameter | Input/Output | Description |
| --- | --- | --- |
| outputs | Input | Vector of output tensor holders. |

## Return Value


| Parameter | Type | Description |
| --- | --- | --- |
| - | std::unique_ptr<Graph> | Returns the smart pointer to the built graph, or nullptr on failure. After the call, the graph builder is no longer usable. |

## Constraints

None

## Examples

-   Build a computation graph.

```c++
    // 1. Create the graph builder (EsGraphBuilder).
    EsGraphBuilder builder("graph_name");
    // 2. Add two input nodes.
    EsTensorHolder [data0, data1] = builder.CreateInputs<2>();
    // 3. Add intermediate nodes. In C++, common operators such as addition, subtraction, multiplication, and division are overloaded and can be used directly.
    EsTensorHolder add = data0 + data1;
    // 4. Set the graph output.
    builder.SetOutput(add, 0);
    // 5. Complete the graph construction and obtain the constructed `Graph` object. Resources in `builder` are destroyed upon destruction.
    std::unique_ptr<ge::Graph> graph = builder.BuildAndReset();
    ```

-   Build a computation graph based on the output tensor list.

    ```
    // 1. Create a graph builder (EsGraphBuilder).
    EsGraphBuilder builder("graph_name");
    // 2. Add two input nodes.
    EsTensorHolder [data0, data1] = builder.CreateInputs<2>();
    // 3. Add intermediate nodes. In C++, common operators such as addition, subtraction, multiplication, and division are overloaded and can be used directly.
    // 4. Set the graph output.
    // 5. Complete the graph construction and obtain the constructed `Graph` object. Resources in `builder` are destroyed upon destruction.
    std::unique_ptr<ge::Graph> graph = builder.BuildAndReset({data0 + data1}); // Complete steps 3, 4, and 5 in one line.
    ```
