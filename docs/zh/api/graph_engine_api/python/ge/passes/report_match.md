# report\_match

## 产品支持情况

全量芯片支持。

## 功能说明

GE融合Pass支持以下两种改图方式：

- 传统直接改图方式：用户在[FusionBasePass.run(graph, context)](FusionBasePass/run.md)中直接调用Graph接口修改原图，例如增删边、删除节点或重连节点。由于改图过程由用户控制，框架无法自动确定结构匹配与融合前后的节点集合。用户需要在发现目标结构后调用本函数上报结构匹配，在改图前调用`can_fuse`执行融合可行性检查，完成自定义改图后、删除旧节点前调用[report_fuse](report_fuse.md)上报融合结果。
- 基于子图替换的方式：这是后续提供的高效改图方式。用户通过boundary描述待替换子图的边界，通过replacement描述替换图，再调用[SubgraphRewriter.replace](SubgraphRewriter/replace.md)完成改图。传入`context`时，该接口会自动完成结构匹配上报、融合可行性检查、子图替换和融合结果上报，无需单独调用本函数、`can_fuse`和`report_fuse`。

本函数用于传统直接改图方式。函数上报一次结构匹配，在图遍历中发现目标结构后调用，无论融合条件是否通过均计入。内部自动累加match\_time，不改变effect\_time，对应信息落盘至fusion\_result.json。

与[report_fuse](report_fuse.md)配合使用可统计结构匹配的命中率：match\_time为结构匹配总次数（全集），effect\_time为融合实际生效次数（子集），match\_time - effect\_time反映因条件过滤而放弃融合的数量。

## 函数原型

```python
report_match(
    matched_nodes: Iterable[Node],
    context: PassContext,
) -> None
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| matched_nodes | 输入 | 结构匹配命中的节点集合，元素必须为同一张图中的ge.graph.Node对象，节点之间需连通。 |
| context | 输入 | 当前Pass的PassContext，由GE注入到`FusionBasePass.run(graph, context)`中。 |

## 返回值说明

成功时返回`None`。节点集合中包含非ge.graph.Node对象，或`context`不是PassContext时抛出`TypeError`；Node handle失效或底层结构匹配上报失败时抛出`RuntimeError`，失败信息会同步到`context`。

## 约束说明

- 该接口应在发现目标结构后、[can_fuse](can_fuse.md)之前调用。
- 必须在旧节点被删除或失效前调用本函数，确保`matched_nodes`仍然有效。
- `matched_nodes`中的节点必须属于同一张图。
- 带`context`的[SubgraphRewriter.replace](SubgraphRewriter/replace.md)内部已自动调用本函数上报结构匹配，调用方使用该重载时无需（不应）再手动调用，否则match\_time会重复累计。手动改图（不走SubgraphRewriter）的场景仍需按上述约束手动调用。
- `PassContext`仅在当前Pass同步调用期间有效，不应保存到实例属性或其他线程中使用。

## 调用示例

以下示例采用传统直接改图方式，删除一个仅有一个数据输入和一个数据输出的中间节点，并将其上游节点直接连接到所有下游节点。发现目标结构后先上报结构匹配，融合条件通过并完成改图后再上报融合结果；条件未通过而放弃融合时仅保留match\_time计数。

```python
from ge.passes import can_fuse, report_fuse, report_match

def remove_intermediate_node(graph, node, context):
    nodes = [node]
    # 发现目标结构后立即上报结构匹配，计入match_time。
    report_match(nodes, context)

    result = can_fuse(nodes)
    if not result.ok:
        # 融合条件未通过而放弃融合，不再调用report_fuse。
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

    # 本次改图未新增节点，因此nodes_after传入空列表。
    # 上报完成前，nodes_before中的节点必须保持有效。
    report_fuse(nodes, [], context)
    graph.remove_node(node)
    return True
```
