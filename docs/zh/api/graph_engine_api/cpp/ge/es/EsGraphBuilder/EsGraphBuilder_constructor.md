# EsGraphBuilder构造函数

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <ge/es\_graph\_builder.h\>
- 库文件：libeager\_style\_graph\_builder\_base.so、libeager\_style\_graph\_builder\_base\_static.a

## 功能说明

构造函数，创建指定名称的图构建器；可指定图后端类别（计算图或执行图）。

## 函数原型

```c++
explicit EsGraphBuilder(const char *name) : graph_builder_(EsCreateGraphBuilder(name), EsDestroyGraphBuilder) {}
EsGraphBuilder(const char *name, GraphKind kind) : graph_builder_(EsCreateGraphBuilderWithKind(name, kind), EsDestroyGraphBuilder) {}
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| name | 输入 | 构建的图名称，如果为nullptr则使用默认名称"graph"。 |
| kind | 输入 | 图后端类别，取值为[C\_GraphKind](https://gitcode.com/cann/metadef/blob/9.3.0/docs/zh/api/c_api/C_GraphKind.md)枚举（`ge::GraphKind`为其别名）：`C_GRAPH_COMPUTE`（计算图，取值为0）、`C_GRAPH_EXECUTE`（执行图，取值为1）。 |

## 返回值说明

无

## 约束说明

- 双参构造接口中，图类别在构建器创建后固定，构建器生命周期内不可变更；`kind`为非法取值时，构建器内部的图构建器指针为空，后续构图接口不可用。
- 不指定图类别的构造接口（`EsGraphBuilder(name)`）默认创建计算图构建器。
- 以执行图类别创建的构建器，部分接口不支持（如[CreateVariable](CreateVariable.md)、[SetShape](../EsTensorHolder/SetShape.md)等TensorDesc写接口），详见对应接口的约束说明。

## 调用示例

```c++
// 1. 创建图构建器（EsGraphBuilder），默认为计算图
EsGraphBuilder builder("graph_name");

// 2. 创建执行图类别的图构建器
EsGraphBuilder execute_builder("graph_name", C_GRAPH_EXECUTE);
```
