# report\_match

<!-- md-trans-meta sourceCommit=9fd182ae1a4bc7e9945ee78eb24f6ad218698348 translatedAt=2026-11-17T02:35:46.741Z pushedAt=2026-11-18T02:41:08.836Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

The GE fusion Pass supports the following two graph modification methods:

- Traditional direct graph modification approach: The user directly calls the Graph API in [FusionBasePass.run(graph, context)](FusionBasePass/run.md) to modify the original graph, for example, by adding or deleting edges, deleting nodes, or reconnecting nodes. Because the graph modification process is controlled by the user, the framework cannot automatically determine the node sets before and after structure matching and merging. The user needs to call this function to report the structure matching after the target structure is found, call `can_fuse` to perform the merge feasibility check before graph modification, and call [report_fuse](report_fuse.md) to report the merge result after completing the custom graph modification and before deleting the old nodes.
- Subgraph replacement-based method: This is an efficient graph modification method provided later. The user describes the boundary of the subgraph to be replaced through boundary, describes the replacement graph through replacement, and then calls [SubgraphRewriter.replace](SubgraphRewriter/replace.md) to complete the graph modification. When `context` is passed in, this API automatically completes the structure matching report, merge feasibility check, subgraph replacement, and merge result report, without the need to separately call this function, `can_fuse`, and `report_fuse`.

This function is used for the traditional direct graph modification approach. The function reports one structure matching and is called after the target structure is found during graph traversal. It is counted regardless of whether the merge condition is met. Internally, it automatically accumulates match\_time, does not change effect\_time, and writes the corresponding information to fusion\_result.json.

When used together with [report_fuse](report_fuse.md), the hit rate of structure matching can be counted: match\_time is the total number of structure matches (the full set), effect\_time is the number of times the merge actually takes effect (the subset), and match\_time - effect\_time reflects the number of merges abandoned due to condition filtering.

## Prototype

```python
report_match(
    matched_nodes: Iterable[Node],
    context: PassContext,
) -> None
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| matched_nodes | Input | Node set hit by structure matching. The elements must be ge.graph.Node objects in the same graph, and the nodes must be connected. |
| context | Input | **PassContext** of the current Pass, injected by GE into `FusionBasePass.run(graph, context)`. |

## Return Value

Returns `None` on success. A `TypeError` is raised when the node set contains objects that are not ge.graph.Node objects, or when `context` is not a PassContext. A `RuntimeError` is raised when the Node handle becomes invalid or when reporting the underlying structure matching fails, and the failure information is synchronized to `context`.

## Constraints

- This API should be called after the target structure is found and before [can_fuse](can_fuse.md).
- This function must be called before the old nodes are deleted or invalidated to ensure that `matched_nodes` remains valid.
- The nodes in `matched_nodes` must belong to the same graph.
- [SubgraphRewriter.replace](SubgraphRewriter/replace.md) with `context` already automatically calls this function internally to report the structure matching. When the caller uses this overload, there is no need to (and it should not) call it manually again; otherwise, match\_time will be accumulated repeatedly. For scenarios where the graph is modified manually (without going through SubgraphRewriter), this function must still be called manually according to the constraints above.
- `PassContext` is valid only during the synchronous invocation of the current Pass and must not be saved to an instance attribute or used in other threads.

## Examples

The following example uses the traditional direct graph modification approach to delete an intermediate node that has only one data input and one data output, and directly connects its upstream node to all downstream nodes. After the target structure is found, the structure matching is reported first; after the merge condition is met and the graph modification is completed, the merge result is reported. When the merge is abandoned because the condition is not met, only the match\_time count is retained.

```python
from ge.passes import can_fuse, report_fuse, report_match

def remove_intermediate_node(graph, node, context):
    nodes = [node]
    # Report the structure matching immediately after the target structure is found, and count it into match_time.
    report_match(nodes, context)

    result = can_fuse(nodes)
    if not result.ok:
        # Abandon the merge because the merge condition is not met, and do not call report_fuse again.
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
    # The nodes in nodes_before must remain valid before the report is completed.
    report_fuse(nodes, [], context)
    graph.remove_node(node)
    return True
```
