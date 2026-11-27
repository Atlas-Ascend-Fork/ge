# InferSymbol

## 产品支持情况

全量芯片支持。该接口在host侧图编译阶段执行，不涉及芯片相关分支。

## 头文件/库文件

- 头文件：\#include <graph/custom\_op.h\>
- 库文件：libgraph.so
- 符号化上下文头文件：`exe_graph/runtime/infer_symbol_context.h`

## 功能说明

符号化推导函数，在编译期根据输入符号Shape和可选符号值推导输出。输入符号Shape由`SymbolShape`描述，维度为常量或`Expression`；数据依赖输入的符号值可通过`SymbolTensor`获取。

## 函数原型

```c++
virtual graphStatus InferSymbol(gert::InferSymbolContext *ctx) = 0
```

## 参数说明

| 参数名 | 输入/输出 | 说明 |
| --- | --- | --- |
| ctx | 输入 | 符号化推导上下文，可读取输入符号Shape/Tensor和属性，并写入输出符号Shape/Tensor。输入符号可能为空指针。 |

## 返回值说明

| 返回值 | 说明 |
| --- | --- |
| GRAPH_SUCCESS | 已提供完整输出符号信息，框架采纳并回写本次结果。返回前必须写入每个输出的符号Shape；标量通过`SetDimNum(0)`表示。符号值仅在数据依赖输出场景需要设置。 |
| GRAPH_NOT_CHANGED | 本次不提供符号信息。框架丢弃本次全部写入并直接降级到静态Shape，不回落内置符号化推导；不表示输出等于输入。 |
| 其他值 | 推导失败，图编译中断，错误日志包含节点名和op type。 |

## 约束说明

- 输入符号缺失时必须判空，无法继续推导时返回`GRAPH_NOT_CHANGED`；透传Shape时必须显式写出输出。
- `EXPECT_SYMBOL_*`仅改变推导分支并生成guard；`ASSERT_SYMBOL_*`失败返回`GRAPH_PARAM_INVALID`，中断编译。
- 回调在符号化pass线程上同步执行，不得跨节点持有上下文中的对象或指针。
- 语言无关式`SymbolicInferOp`优先于宏式/OpDef式字段注册；两种方式同存时使用本接口。
- 未实现该接口不影响算子的其他编译和执行能力。

## 调用示例

```cpp
class AddCustom : public SymbolicInferOp {
 public:
  graphStatus InferSymbol(gert::InferSymbolContext *ctx) override {
    const auto *x = ctx->GetInputSymbolShape(0);
    auto *out = ctx->GetOutputSymbolShape(0);
    if (x == nullptr || out == nullptr) {
      return GRAPH_NOT_CHANGED;
    }
    *out = *x;
    return GRAPH_SUCCESS;
  }
};

REG_AUTO_MAPPING_OP(AddCustom);
```
