# --topo\_sorting\_mode

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-22T03:44:56.313Z pushedAt=2026-10-27T03:55:43.538Z -->

## Product Support

Full chip support

## Feature Description

Different graph traversal modes that can be selected when compiling operators in graph mode.

## Related Parameters

None

## Parameter Values

- 0: BFS, Breadth First Search, a breadth-first traversal strategy. It is a search algorithm similar to DFS. Starting from a state, it searches all reachable states. Unlike DFS, BFS traverses layer by layer.
- 1: (default) DFS, Depth First Search, a depth-first traversal strategy. It is a search algorithm that starts from a state and keeps transitioning to new states until no further transition is possible, then backtracks to the previous state and continues transitioning to other states, repeating this process until the final solution is found.
- 2: RDFS, Reverse DFS, a reverse depth-first traversal strategy.
- 3: StableRDFS, a stable topological order strategy. For existing operators in the graph, it does not change their computation order. For newly added operators in the graph, it uses the RDFS traversal strategy.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --topo_sorting_mode=3 ...
```

## Usage Constraints

During model conversion, if a custom knowledge base path is loaded, changes to the graph traversal mode may affect the tuning result. It is recommended that you perform tuning again. For details, see [*AOE Tuning Tool*](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/devaids/aoe/auxiliarydevtool_aoe_0001.html).
