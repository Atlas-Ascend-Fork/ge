# --topo\_sorting\_mode

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-11-16T14:41:50.498Z pushedAt=2026-11-17T12:22:26.701Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

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

## Constraints

During model conversion, if a custom knowledge base path is loaded, changes to the graph traversal mode may affect the tuning result. It is recommended that you perform tuning again. For details, see [*AOE Tuning Tool*](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/devaids/aoe/auxiliarydevtool_aoe_0001.html).
