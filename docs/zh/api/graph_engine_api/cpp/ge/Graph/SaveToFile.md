# SaveToFile

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <graph/graph.h\>
- 库文件：libgraph.so

## 功能说明

将序列化后的Graph结构保存到文件中。

## 函数原型

> [!NOTE]说明
>数据类型为string的接口后续版本会废弃，建议使用数据类型为非string的接口。

```c++
graphStatus SaveToFile(const std::string &file_name) const
graphStatus SaveToFile(const char_t *file_name) const
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| file_name | 输入 | 需要保存的文件路径和文件名。 |

## 返回值说明

| 参数名 | 类型 | 说明 |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0)：成功。<br>其他值：失败。 |

## 约束说明

- 保存的文件格式为air格式，可以通过[LoadFromFile](LoadFromFile.md)接口还原为graph对象。
- 仅计算图支持该接口：当Graph以执行图类别构造（`Graph(name, C_GRAPH_EXECUTE)`）时，调用该接口返回GRAPH_FAILED，并上报错误码E18888。执行图不支持序列化保存，如需查看图结构，可使用[Dump](Dump.md)或[DumpToFile](DumpToFile.md)接口。
