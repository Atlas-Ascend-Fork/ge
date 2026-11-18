# SaveToMem

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <graph/graph.h\>
- 库文件：libgraph.so

## 功能说明

将序列化后的Graph保存到内存中。

## 函数原型

```c++
graphStatus SaveToMem(GraphBuffer &graph_buffer) const
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| graph_buffer | 输出 | 保存的Graph的buffer。 |

## 返回值说明

| 参数名 | 类型 | 说明 |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0)：成功。<br>其他值：失败。 |

## 约束说明

- 内存中保存的内容格式为air格式，可以通过[LoadFromMem](LoadFromMem.md)接口还原为graph对象。
- 仅计算图支持该接口：当Graph以执行图类别构造（`Graph(name, C_GRAPH_EXECUTE)`）时，调用该接口返回GRAPH_FAILED，并上报错误码E18888。
