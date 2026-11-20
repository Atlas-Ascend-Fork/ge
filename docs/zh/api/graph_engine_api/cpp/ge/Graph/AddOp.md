# AddOp

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <graph/graph.h\>
- 库文件：libgraph.so

## 功能说明

用户算子缓存接口，通过此接口可以将不带连接关系的算子缓存在图中，用于查询和对象获取。

## 函数原型

```c++
graphStatus AddOp(const ge::Operator &op)
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| op | 输入 | 需增加的算子。 |

## 返回值说明

| 参数名 | 类型 | 说明 |
| --- | --- | --- |
| - | graphStatus | SUCCESS：操作成功<br>FAILED：操作失败 |

## 约束说明

- 此接口为非必须接口，主要用于跨函数共享Operator对象：用户在多个函数间切换构图时，Operator对象可能因作为某个函数的局部变量而无法被其他函数访问，此时可将Operator注册到Graph中，其他函数再通过Graph获取先前创建的Operator对象。Operator一经注册即由Graph持有，在Graph销毁前会一直存在。
- 仅计算图支持该接口：当Graph以执行图类别构造（`Graph(name, C_GRAPH_EXECUTE)`）时，调用该接口返回GRAPH_FAILED，并上报错误码E18888。
