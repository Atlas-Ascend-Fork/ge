# NodeCost

<!-- md-trans-meta sourceCommit=76a2946a47b1d8a6a7bedfc5a97e11334c04e83e translatedAt=2026-11-17T02:36:50.892Z pushedAt=2026-11-18T02:41:08.837Z nl=2 -->

## Prototype

```python
multistream.NodeCost(
    *, execution_time: float = -1.0,
    memory_usage: int = 0, bandwidth_usage: int = 0,
    cube_block_num: int = 0, vec_block_num: int = 0,
    cube_core_num_limit: int = 0, vec_core_num_limit: int = 0,
)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| execution_time | Input | Bounded duration value, stored as float32; -1.0 indicates an unknown duration, which still participates in traffic distribution. |
| memory_usage / bandwidth_usage | Input | Non-negative integers for memory and bandwidth usage, defaulting to 0. |
| cube_block_num / vec_block_num | Input | Number of Cube/Vector cores actually occupied, non-negative integers, defaulting to 0. |
| cube_core_num_limit / vec_core_num_limit | Input | Upper limit of user cores, non-negative integers, where 0 indicates no limit. |

## Return Value

Returns an immutable `NodeCost` object for use by [DAGNode.set_node_cost](DAGNode.md).

## Constraints

- Only keyword parameters are accepted, and fields cannot be modified after construction; a new object is created when adjusting the cost.
- bool is not a valid value; an invalid type raises `TypeError`, and a non-bounded duration, float32 overflow, or negative integer parameter raises `ValueError`.
- A normal node does not participate in traffic distribution when the absolute value of its duration does not exceed `1e-6`.

## Examples

```python
import multistream as ms

cost = ms.NodeCost(execution_time=10.0, memory_usage=1024)
graph = ms.DAGGraph("example")
node = graph.add_node("compute", "Relu")
node.set_node_cost(cost)
assert node.get_node_cost().memory_usage == 1024
```
