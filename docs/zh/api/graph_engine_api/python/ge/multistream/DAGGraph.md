# DAGGraph

## 函数原型

```python
multistream.DAGGraph(name: str)
add_node(name: str, type: str) -> Optional[DAGNode]
find_node(name: str) -> Optional[DAGNode]
add_edge(src: DAGNode, src_port: int, dst: DAGNode, dst_port: int) -> None
get_name() -> str
get_nodes() -> List[DAGNode]
get_node_count() -> int
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| name | 输入 | 图名或节点名，字符串。节点名在图内唯一。 |
| type | 输入 | 节点类型，字符串。 |
| src / dst | 输入 | 依赖边的源节点和目标节点，均须属于当前图。 |
| src_port / dst_port | 输入 | 整数端口：数据依赖使用非负整数，控制依赖可使用`-1`；不接受bool。 |

## 返回值说明

构造函数返回图对象。`add_node()`返回[DAGNode](DAGNode.md)，同名节点已存在时返回`None`；`find_node()`未找到节点时返回`None`。`add_edge()`返回`None`。

`get_name()`返回图名；`get_nodes()`按插入顺序返回节点列表副本；`get_node_count()`返回节点数。

## 约束说明

- 参数类型错误时抛出`TypeError`。
- 边端点不属于当前图或端口小于`-1`时抛出`ValueError`。
- 数据和控制依赖均须完整表达；环检测由[plan](plan.md)执行。
- 修改`get_nodes()`返回的列表不改变图结构，其中的节点仍为原图节点。

## 调用示例

```python
import multistream as ms

graph = ms.DAGGraph("example")
source = graph.add_node("source", "Data")
target = graph.add_node("target", "Relu")
graph.add_edge(source, 0, target, 0)
assert graph.find_node("target") is target
assert graph.get_node_count() == 2
```
