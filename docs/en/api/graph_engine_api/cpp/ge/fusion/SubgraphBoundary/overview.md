# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:41:43.706Z pushedAt=2026-10-28T07:40:22.615Z nl=2 -->

Used to define the **Subgraph** boundary:

- A **Subgraph** must be self-contained except for its boundary output operators. All data output consumers of operators within the boundary must also be within the boundary. If there are consumers outside the boundary, the consumers will fail to find the original **Tensor** after rewriting.
- Nodes in a **Subgraph** cannot span graphs.
- A **Subgraph** cannot contain the boundary operators of the graph it belongs to (such as **data**/**netoutput**/**variable**).

## Header File to Include

```c++
#include <ge/fusion/subgraph_boundary.h>
```

## Public Member Functions

```c++
SubgraphBoundary()
SubgraphBoundary(std::vector<SubgraphInput> inputs, std::vector<SubgraphOutput> outputs)
SubgraphBoundary(const SubgraphBoundary &other) noexcept
SubgraphBoundary &operator=(SubgraphBoundary &&other) noexcept
SubgraphBoundary &operator=(const SubgraphBoundary &other) noexcept
Status AddInput(int64_t index, SubgraphInput input)
Status AddOutput(int64_t index, SubgraphOutput output)
Status GetInput(int64_t index, SubgraphInput &subgraph_input) const
Status GetAllInputs(std::vector<SubgraphInput> &subgraph_input) const
Status GetOutput(int64_t index, SubgraphOutput &subgraph_output) const
Status GetAllOutputs(std::vector<SubgraphOutput> &subgraph_outputs) const
```
