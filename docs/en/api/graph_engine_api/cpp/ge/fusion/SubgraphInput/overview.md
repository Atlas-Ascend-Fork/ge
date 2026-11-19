# Overview

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T13:42:54.856Z pushedAt=2026-10-28T07:40:22.622Z nl=2 -->

Describes an input **Tensor** of a **Subgraph**.

- Consider that a **Tensor** may have multiple consumers, so a **Subgraph** input may correspond to multiple **Node** inputs.

    ```plaintext
           tensor1
            /  \
          op1   op2
    ```

    As shown in the example above, tensor1 is an input of a **Subgraph**, and it corresponds to two nodes, op1 and op2, on the boundary. tensor1 is outside the **Subgraph** boundary, while op1 and op2 are inside the **Subgraph** boundary.

- Consider that tensor1 has two consumers, where op1 is inside the **Subgraph** boundary and op2 is outside the boundary.

    Therefore, when describing a **Subgraph** input, it is necessary to clarify the **Node** inputs on the boundary.

## Header File to Include

```c++
#include <ge/fusion/subgraph_boundary.h>
```

## Public Member Functions

```c++
SubgraphInput()
explicit SubgraphInput(std::vector<NodeIo> node_inputs)
SubgraphInput(const SubgraphInput &other) noexcept
SubgraphInput &operator=(SubgraphInput &&other) noexcept
SubgraphInput &operator=(const SubgraphInput &other) noexcept
Status AddInput(const NodeIo &node_input)
[[nodiscard]] std::vector<NodeIo> GetAllInputs() const
```
