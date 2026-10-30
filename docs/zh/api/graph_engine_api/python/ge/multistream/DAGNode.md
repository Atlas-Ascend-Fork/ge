# DAGNode

## 函数原型

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

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| label | 输入 | 串行组标签，默认空字符串。 |
| cost | 输入 | [NodeCost](NodeCost.md)对象。 |
| core_type | 输入 | [CoreType](CoreType.md)枚举成员，默认`UNKNOWN`。 |

## 返回值说明

`get_name()`、`get_type()`、`get_index()`分别返回名称、类型和图内插入索引。其余getter返回对应标签、成本副本或执行引擎类型；setter返回`None`。

## 约束说明

- 通过[DAGGraph.add_node](DAGGraph.md)创建节点。
- setter参数类型不正确时抛出`TypeError`；执行引擎类型应传枚举成员，不传字符串。
- 同一非空标签的节点分到同一条额外串行流；不同标签按首次出现顺序追加在普通流之后。
- 串行组节点即使耗时为零，仍会分配到额外串行流。

## 调用示例

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
