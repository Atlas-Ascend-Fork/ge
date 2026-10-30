# replace

<!-- md-trans-meta sourceCommit=ee6d5ce9f7bcc0195885ddb239de680870e3e564 translatedAt=2026-10-15T04:09:12.384Z pushedAt=2026-10-16T10:53:07.688Z -->

## Applicable Products

Supported on all chips.

## Description

Replaces the subgraph described by **boundary** with **replacement**. This is a static method.

## Prototype

```python
@staticmethod
replace(boundary: SubgraphBoundary, replacement: Graph) -> int

@staticmethod
replace(boundary: SubgraphBoundary, replacement: Graph, *, context: PassContext) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| boundary | Input | Subgraph boundary, of type **SubgraphBoundary**, describing the input/output of the subgraph to be replaced. |
| replacement | Input | Replacement graph, of type **ge.graph.Graph**. |
| context | Input | Optional. **PassContext** of the current pass, injected by GE into `FusionBasePass.run(graph, context)`. After being passed in, the underlying layer automatically performs the fusion feasibility check, subgraph replacement, and fusion result reporting. |

## Return Value

| Type | Description |
| --- | --- |
| int | When `context` is not passed in, returns the integer form of the C++ Status. On failure, check the log for information about boundary integrity and index alignment. |
| None | When `context` is passed in, returns `None` on success. Throws `RuntimeError` when the replacement graph handle becomes invalid, or when the fusion feasibility check, subgraph replacement, or fusion result reporting fails. The failure information is synchronized to `context`. |

## Constraints

- **replacement** must be a non-empty graph wrapper. The graph is **copied** on the C++ side, but the GE rules for Python **Graph** ownership still apply.
- It is recommended to pass in `context` in `FusionBasePass.run(graph, context)` so that the underlying layer completes the fusion feasibility check and result reporting.
- When `context` is not passed in, the original behavior is retained, and fusion result reporting is not automatically performed.

## Examples

The logical differences between passing in `context` and not passing in `context` are as follows:

| Call Mode | Fusion Feasibility Check and Result Reporting | Success Return Value | Failure Handling |
| --- | --- | --- | --- |
| `context` not passed in | Retains the original behavior and does not automatically perform the fusion feasibility check and fusion result reporting newly added by this API. | The integer form of the C++ `Status` | The user checks the return value. |
| `context` passed in | Automatically performs the fusion feasibility check and fusion result reporting. | `None` | Throws `RuntimeError` and tries to synchronize the failure information to `context`. |

```python
from ge.passes import SubgraphBoundary, SubgraphInput, SubgraphOutput, SubgraphRewriter

# The following code is located in FusionBasePass.run(graph, context).
# n0 and out_node are existing nodes in the main graph, and replacement_graph is the constructed replacement graph.
b = SubgraphBoundary()
b.add_input(0, SubgraphInput([(n0, 0), (n0, 1)]))
b.add_output(0, SubgraphOutput(out_node, 0))
```

After completing the construction of `boundary`, choose one of the following two call modes:

### Context Not Passed In

Retains the original behavior, where the user checks the return value.

```python
ret = SubgraphRewriter.replace(b, replacement_graph)
if ret != 0:
    raise RuntimeError(f"SubgraphRewriter.replace failed, ret={ret}")
```

### Passing in Context

Automatically performs the fusion feasibility check and reports the fusion result. Returns `None` on success and raises `RuntimeError` on failure.

```python
SubgraphRewriter.replace(b, replacement_graph, context=context)
```
