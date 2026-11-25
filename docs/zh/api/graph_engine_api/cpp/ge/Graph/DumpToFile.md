# DumpToFile

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <graph/graph.h\>
- 库文件：libgraph.so

## 功能说明

将graph序列化到执行路径下的文件中。

不包含权重等数据，只包含图结构及相关属性。

## 函数原型

```c++
graphStatus DumpToFile(DumpFormat format, const AscendString &suffix) const
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| format | 输入 | dump文件的格式，有onnx、txt等。DumpFormat枚举请参见[DumpFormat](../DumpFormat.md)。 |
| suffix | 输入 | 文件名后缀，添加在文件名末尾（扩展名前），例如 `ge_proto_xxxx_GraphPreRunBegin.txt` 中的 `GraphPreRunBegin`。 |

## 返回值说明

| 参数名 | 类型 | 说明 |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0)：成功。<br>其他值：失败。 |

## 约束说明

计算图与执行图均支持该接口。以执行图类别构造的Graph对象调用该接口时，内部会将执行图转换为等价的计算图只读快照后输出，快照不会回写，不影响原图结构。
