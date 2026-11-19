# DAGGraph

<!-- md-trans-meta sourceCommit=76a2946a47b1d8a6a7bedfc5a97e11334c04e83e translatedAt=2026-11-17T02:35:10.950Z pushedAt=2026-11-18T02:41:08.830Z nl=2 -->

## Prototype

```python
multistream.DAGGraph(name: str)
add_node(name: str, type: str) -> Optional[DAGNode]
find_node(name: str) -> Optional[DAGNode]
add_edge(src: DAGNode, src_port: int, dst: DAGNode, dst_port: int) -> None
get_name() -> str
get_nodes() -> List[DAGNode]
get_node_count() -> int
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Graph name or node name, a string. The node name must be unique within the graph. |
| type | Input | Node type, a string. |
| src / dst | Input | Source node and destination node of the dependency edge. Both must belong to the current graph. |
| src_port / dst_port | Input | Integer port. A non-negative integer is used for data dependency, and `-1` can be used for control dependency. bool is not accepted. |

## Return Value

The constructor returns a graph object. `add_node()` returns [DAGNode](DAGNode.md), or `None` if a node with the same name already exists. `find_node()` returns `None` if the node is not found. `add_edge()` returns `None`.

`get_name()` returns the graph name. `get_nodes()` returns a copy of the node list in insertion order. `get_node_count()` returns the number of nodes.

## Constraints

- Throws `TypeError` when a parameter type is incorrect.
- Throws `ValueError` when an edge endpoint does not belong to the current graph or a port is less than `-1`.
- Both data dependency and control dependency must be fully expressed. Cycle detection is performed by [plan](plan.md).
- Modifying the list returned by `get_nodes()` does not change the graph structure, and the nodes in the list remain the nodes of the original graph.

## Examples

```python
import multistream as ms

graph = ms.DAGGraph("example")
source = graph.add_node("source", "Data")
target = graph.add_node("target", "Relu")
graph.add_edge(source, 0, target, 0)
assert graph.find_node("target") is target
assert graph.get_node_count() == 2
```
