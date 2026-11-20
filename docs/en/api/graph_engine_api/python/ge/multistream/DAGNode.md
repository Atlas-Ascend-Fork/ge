# DAGNode

<!-- md-trans-meta sourceCommit=76a2946a47b1d8a6a7bedfc5a97e11334c04e83e translatedAt=2026-11-17T02:35:35.187Z pushedAt=2026-11-18T02:41:08.831Z nl=2 -->

## Prototype

```python
get_name() -> str
get_type() -> str
get_index() -> int
set_stream_label(label: str) -> None
get_stream_label() -> str
set_node_cost(cost: NodeCost) -> None
get_node_cost() -> NodeCost
set_core_type(core_type: CoreType) -> None
get_core_type() -> CoreType
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| label | Input | Serial group label, defaulting to an empty string. |
| cost | Input | [NodeCost](NodeCost.md) object. |
| core_type | Input | [CoreType](CoreType.md) enumeration member, defaulting to `UNKNOWN`. |

## Return Value

`get_name()`, `get_type()`, and `get_index()` return the name, type, and insertion index within the graph, respectively. The other getters return the corresponding label, a copy of the cost, or the execution engine type; setters return `None`.

## Constraints

- Nodes are created through [DAGGraph.add_node](DAGGraph.md).
- A `TypeError` is raised when a setter parameter has an incorrect type; the execution engine type must be passed as an enumeration member, not a string.
- Nodes with the same non-empty label are assigned to the same additional serial stream; different labels are appended after the normal stream in the order of their first occurrence.
- Serial group nodes are still assigned to an additional serial stream even if their cost is zero.

## Examples

```python
import multistream as ms

graph = ms.DAGGraph("example")
node = graph.add_node("compute", "Relu")
node.set_stream_label("serial_group")
node.set_node_cost(ms.NodeCost(execution_time=0.0))
node.set_core_type(ms.CoreType.AIV)
assert node.get_stream_label() == "serial_group"
assert node.get_node_cost().execution_time == 0.0
assert node.get_core_type() == ms.CoreType.AIV
```
