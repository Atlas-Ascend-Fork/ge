# NodeCost

## 函数原型

```python
multistream.NodeCost(
    *, execution_time: float = -1.0,
    memory_usage: int = 0, bandwidth_usage: int = 0,
    cube_block_num: int = 0, vec_block_num: int = 0,
    cube_core_num_limit: int = 0, vec_core_num_limit: int = 0,
)
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| execution_time | 输入 | 有限耗时数值，存储为float32；-1.0表示未知耗时，仍参与分流。 |
| memory_usage / bandwidth_usage | 输入 | 非负整数，内存和带宽用量，默认0。 |
| cube_block_num / vec_block_num | 输入 | 实际占用的Cube / Vector核数，非负整数，默认0。 |
| cube_core_num_limit / vec_core_num_limit | 输入 | 用户核数上限，非负整数，0表示不限制。 |

## 返回值说明

返回不可变的`NodeCost`对象，供[DAGNode.set_node_cost](DAGNode.md)使用。

## 约束说明

- 仅接受关键字参数，不能修改构造后的字段；调整成本时创建新对象。
- bool不作为合法数值；类型不合法抛出`TypeError`，非有限耗时、float32溢出或负整数参数抛出`ValueError`。
- 普通节点耗时绝对值不超过`1e-6`时不参与分流。

## 调用示例

```python
import multistream as ms

cost = ms.NodeCost(execution_time=10.0, memory_usage=1024)
graph = ms.DAGGraph("example")
node = graph.add_node("compute", "Relu")
node.set_node_cost(cost)
assert node.get_node_cost().memory_usage == 1024
```
