# SetStorageShape

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <ge/es\_c\_tensor\_holder.h\>
- 库文件：libeager\_style\_graph\_builder\_base.so、libeager\_style\_graph\_builder\_base\_static.a

## 功能说明

设置存储形状。

## 函数原型

```c++
ge::Status SetStorageShape(const ge::Shape &shape)
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| shape | 输入 | 存储形状。 |

## 返回值说明

| 参数名 | 类型 | 说明 |
| --- | --- | --- |
| - | ge::Status | 操作状态。<br><br>  - 0：成功<br>  - 1：失败 |

## 约束说明

仅计算图支持该接口：当张量所属的图构建器以执行图类别创建（[EsCGraphBuilder](../EsCGraphBuilder/EsCGraphBuilder_constructor_and_destructor.md)构造时`kind`为`C_GRAPH_EXECUTE`）时，调用该接口失败，并上报错误码E10062。
