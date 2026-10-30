# can\_fuse

<!-- md-trans-meta sourceCommit=7e558b3d0f99a4068a1a06cd58d5e85d7df2626b translatedAt=2026-10-15T03:40:04.651Z pushedAt=2026-10-16T10:53:07.581Z -->

## Applicable Products

Supported on all chips.

## Description

Before modifying the graph, checks whether a group of main graph nodes satisfies the fusion constraints, without modifying the graph. The checks include:

- The node set is non-empty, and each **Node** and its underlying handle are valid.
- All nodes have a valid owner graph and belong to the same graph.
- The user stream labels already set on the nodes must not contain multiple different values.
- The SuperKernel scopes and SuperKernel options already set on the nodes must not contain multiple different values, respectively.
- The AI Core count configurations already set on the nodes must not contain multiple different values, and the Vector Core count configurations must not contain multiple different values either.
- After fusing the node set into a single node, the data edges and control edges do not form a cycle.

## Prototype

```python
can_fuse(nodes: Iterable[Node]) -> FuseCheckResult
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **nodes** | Input | Set of connected nodes to be fused. Elements must be **ge.graph.Node** objects in the same graph. Iterables such as lists, tuples, and generators are supported. |

## Return Value

| Type | Description |
| --- | --- |
| FuseCheckResult | Returns `FuseCheckResult(ok=True, reason="")` when all checks pass; returns `FuseCheckResult(ok=False, reason=<reason>)` when any of the above checks fails. This API does not output the reason automatically. You need to check `reason` yourself and output it as needed. |

A `TypeError` is raised when `nodes` contains objects that are not `ge.graph.Node`; a `RuntimeError` is raised when the node handle is invalid.

The reason for fusion failure is output by the user. For example:

```python
from ge.passes import can_fuse

def fuse_nodes(nodes_before_fuse):
    # Perform the can_fuse check.
    result = can_fuse(nodes_before_fuse)
    if not result.ok:
        print(f"[FuseNodes] can_fuse check failed: {result.reason}")
        return False
    return True
```

## Constraints

- The nodes in `nodes` must belong to the same graph.
- `nodes` must not be empty and must be a connected set of nodes to be fused.
- No exception is thrown when fusion is not possible for business reasons. Check the `ok` and `reason` fields of the returned result.

## Examples

`context` is injected by GE when executing the pass. It is the second parameter of `FusionBasePass.run(graph, context)` and does not need to be created by the user.

```python
from ge.passes import can_fuse

def run(self, graph, context):
    nodes_before = ...  # Nodes to be fused selected from the graph.
    result = can_fuse(nodes_before)
    if not result.ok:
        context.set_error_message(result.reason)
        return False
    ...
```
