# EsCreateGraphBuilderWithKind

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <ge/esb\_funcs.h\>
- 库文件：libeager\_style\_graph\_builder\_base.so、libeager\_style\_graph\_builder\_base\_static.a

## 功能说明

创建一个指定后端类别的图构建器实例。图类别决定构建器生成的图类型：计算图（`C_GRAPH_COMPUTE`）或执行图（`C_GRAPH_EXECUTE`）。

## 函数原型

```c
EsCGraphBuilder *EsCreateGraphBuilderWithKind(const char *name, int32_t kind)
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| name | 输入 | 可选，图的名字，如果为空则默认为"graph"。 |
| kind | 输入 | 图类别，按整型直传`C_GraphKind`枚举取值（定义于`graph/c_types.h`头文件）：`C_GRAPH_COMPUTE`为计算图，`C_GRAPH_EXECUTE`为执行图。 |

## 返回值说明

| 参数名 | 类型 | 说明 |
| --- | --- | --- |
| - | EsCGraphBuilder * | 返回图构建器指针，`kind`为非法取值时返回nullptr。 |

## 约束说明

- 图类别在构建器创建后固定，构建器生命周期内不可变更。
- `kind`仅支持`C_GRAPH_COMPUTE`、`C_GRAPH_EXECUTE`两个取值，其他取值返回nullptr。
- 以执行图类别创建的构建器，部分接口不支持（如EsCreateVariable、TensorDesc写接口等），详见对应接口的约束说明。

## 调用示例

```c
#include "c_types.h" // 包含C_GraphKind枚举定义
// 创建执行图类别的图构建器
EsCGraphBuilder *builder = EsCreateGraphBuilderWithKind("graph_name", C_GRAPH_EXECUTE);
```
