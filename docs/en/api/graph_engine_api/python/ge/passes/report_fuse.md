# report\_fuse

<!-- md-trans-meta sourceCommit=ee6d5ce9f7bcc0195885ddb239de680870e3e564 translatedAt=2026-10-15T03:51:05.989Z pushedAt=2026-10-16T10:53:07.601Z -->

## Applicable Products

Supported on all chips.

## Description

The GE fusion Pass supports the following two graph modification methods:

- Traditional direct graph modification: In [FusionBasePass.run(graph, context)](FusionBasePass/run.md), you directly call graph APIs to modify the original graph, for example, adding or deleting edges, deleting nodes, or reconnecting nodes. Because the graph modification process is controlled by you, the framework cannot automatically determine the node sets before and after fusion. You need to call `can_fuse` to perform a fusion feasibility check before modifying the graph, and call this function to report the fusion result after completing the custom graph modification and before deleting the old nodes.
- Subgraph replacement-based method: This is an efficient graph modification method provided later. You describe the boundary of the subgraph to be replaced through boundary, describe the replacement graph through replacement, and then call [SubgraphRewriter.replace](SubgraphRewriter/replace.md) to complete the graph modification. When `context` is passed in, this API automatically completes the fusion feasibility check, subgraph replacement, and fusion result reporting, without the need to separately call `can_fuse` and this function.

This function is used for the traditional direct graph modification method. It updates the fusion maintenance and debugging information based on the pre-fusion and post-fusion node sets passed in by the user, and records the name of the current Pass.

## Prototype

```python
report_fuse(
    nodes_before: Iterable[Node],
    nodes_after: Iterable[Node],
    context: PassContext,
) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| nodes_before | Input | Connected node set before fusion. Elements must be **ge.graph.Node** objects. |
| nodes_after | Input | Connected node set after fusion. Elements must be **ge.graph.Node** objects. An empty set indicates that only old nodes are deleted and no new nodes are added. |
| context | Input | **PassContext** of the current Pass, injected by GE into `FusionBasePass.run(graph, context)`. |

## Return Value

Returns `None` on success. A `TypeError` is raised when the node set contains objects that are not **ge.graph.Node** objects, or when `context` is not a **PassContext**. A `RuntimeError` is raised when the Node handle becomes invalid or when reporting the underlying fusion result fails, and the failure information is synchronized to `context`.

## Constraints

- Custom graph modification must be completed before calling this function.
- This function must be called before the old nodes are deleted or invalidated to ensure that `nodes_before` remains valid.
- The nodes in `nodes_before` must belong to the same graph; a non-empty `nodes_after` must also belong to that graph.
- `PassContext` is valid only during the synchronous invocation of the current Pass and must not be saved to an instance attribute or used in other threads.

## Examples

The following example uses the traditional direct graph modification approach to delete an intermediate node that has only one data input and one data output, and directly connects its upstream node to all downstream nodes.

```python
from ge.passes import can_fuse, report_fuse

def remove_intermediate_node(graph, node, context):
    nodes_before = [node]
    result = can_fuse(nodes_before)
    if not result.ok:
        context.set_error_message(result.reason)
        return False

    input_node, input_output_index = node.get_in_data_nodes_and_port_indexes(0)
    consumers = list(node.get_out_data_nodes_and_port_indexes(0))

    graph.remove_edge(input_node, input_output_index, node, 0)
    for consumer, consumer_input_index in consumers:
        graph.remove_edge(node, 0, consumer, consumer_input_index)
        graph.add_data_edge(
            input_node,
            input_output_index,
            consumer,
            consumer_input_index,
        )

    # No nodes are added in this graph modification, so an empty list is passed to nodes_after.
    # Before the report is completed, the nodes in nodes_before must remain valid.
    report_fuse(nodes_before, [], context)
    graph.remove_node(node)
    return True
```
