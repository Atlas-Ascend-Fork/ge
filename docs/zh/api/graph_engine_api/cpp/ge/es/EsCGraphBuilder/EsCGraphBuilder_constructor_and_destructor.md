# EsCGraphBuilder构造函数和析构函数

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <ge/es\_c\_graph\_builder.h\>
- 库文件：libeager\_style\_graph\_builder\_base.so、libeager\_style\_graph\_builder\_base\_static.a

## 功能说明

EsCGraphBuilder构造函数和析构函数。

## 函数原型

```c++
EsCGraphBuilder()
explicit EsCGraphBuilder(const char *name)
EsCGraphBuilder(const char *name, ge::GraphKind kind)
~EsCGraphBuilder()
EsCGraphBuilder(EsCGraphBuilder &&) noexcept
EsCGraphBuilder &operator=(EsCGraphBuilder &&) noexcept
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| name | 输入 | 图名称，按照指定的名称构造图构建器。 |
| kind | 输入 | 图后端类别，取值为`ge::GraphKind`（即`C_GraphKind`枚举，定义于`graph/c_types.h`头文件）：`C_GRAPH_COMPUTE`（计算图）、`C_GRAPH_EXECUTE`（执行图）。 |

## 返回值说明

无

## 约束说明

- 双参构造接口中，图类别在构建器创建后固定，构建器生命周期内不可变更，需保证`name`为合法名称、`kind`为合法取值（`C_GRAPH_COMPUTE`或`C_GRAPH_EXECUTE`）。
- 不指定图类别的构造接口（`EsCGraphBuilder()`、`EsCGraphBuilder(name)`）默认创建计算图构建器。
- 以执行图类别创建的构建器，部分接口不支持（如EsCreateVariable、EsSetShape等TensorDesc写接口），详见对应接口的约束说明。
