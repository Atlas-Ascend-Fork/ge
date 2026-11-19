# Graph构造函数和析构函数

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <graph/graph.h\>
- 库文件：libgraph.so

## 功能说明

Graph构造函数和析构函数。

## 函数原型

> [!NOTE]说明
>数据类型为string的接口后续版本会废弃，建议使用数据类型为非string且传name参数的接口。默认无参的构造函数构造的是一个非法的Graph对象。

```c++
explicit Graph(const std::string& name)
explicit Graph(const char *name)
Graph(const char *name, GraphKind kind)
Graph()
~Graph()
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| name | 输入 | Graph名称，按照指定的名称构造Graph。 |
| kind | 输入 | 图后端类别，取值为`C_GraphKind`枚举（定义于`graph/c_types.h`头文件，`ge::GraphKind`为其别名）：`C_GRAPH_COMPUTE`（计算图，取值为0）、`C_GRAPH_EXECUTE`（执行图，取值为1）。 |

## 返回值说明

Graph构造函数返回Graph类型的对象。

## 约束说明

- 图类别在构造期固定，Graph对象生命周期内不可变更。
- 双参构造接口中，`name`为空指针或`kind`为非法取值（非`C_GRAPH_COMPUTE`、`C_GRAPH_EXECUTE`）时，构造出的Graph对象非法，即[IsValid](IsValid.md)返回false。
- 按名称构造的接口（`Graph(name)`）默认创建计算图。
- 以执行图类别构造的Graph对象，部分门面接口不支持（如[SetTargets](SetTargets.md)、[AddOp](AddOp.md)、序列化保存/加载等），详见对应接口的约束说明。
