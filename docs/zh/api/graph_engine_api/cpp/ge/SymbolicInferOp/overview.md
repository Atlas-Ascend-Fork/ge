# 简介

自定义算子的符号化推导能力接口，适用于动态Shape图在编译期进行符号Shape和符号值推导的场景。实现该接口后，GE通过`CustomOpCast<SymbolicInferOp>`自动发现该能力，无需额外注册回调。

本接口及示例需与包含`SymbolicInferOp`、`InferSymbolContext`及配套符号化类型的GE/metadef版本配套使用；老版本可能仅保证算子SO加载成功，接口能力可能不生效。

## 需要包含的头文件

```c++
#include <graph/custom_op.h>
```

符号化推导上下文定义于`exe_graph/runtime/infer_symbol_context.h`。

## Public成员函数

```c++
virtual graphStatus InferSymbol(gert::InferSymbolContext *ctx) = 0
```
