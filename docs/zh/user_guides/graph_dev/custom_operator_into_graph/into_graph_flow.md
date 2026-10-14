# 入图流程

在执行入图操作前，需先定位执行场景并确定前端框架，随后根据不同阶段开展具体操作。

## 1. 先定位执行场景

| 场景 | 说明 | 接口组合 | 适用框架 |
| --- | --- | --- | --- |
| 场景1：动态图在线执行 | kernel已编译好，直接入图执行 | EagerExecuteOp+ShapeInferOp（可选） | 所有框架 |
| 场景2：在线编译+在线执行 | 图编译期RTC编译kernel再执行 | EagerExecuteOp+CompilableOp+ShapeInferOp(可选) | 所有框架 |
| 场景3：离线OM模型下沉 | 编译产物序列化进OM，加载恢复 | EagerExecuteOp+CompilableOp+ShapeInferOp+PortableOp | 所有框架 |
| 场景4：数据依赖shape | 输出shape依赖运行期数据 | EagerExecuteOp+ShapeInferOp | 所有框架 |

执行场景详细介绍请参见[执行场景介绍](concept_and_design.md#执行场景介绍)。执行场景与回调接口对应关系如下，回调接口详细介绍请参见[回调接口详解](callback_interface.md)，此处仅列举触发时机。

| 回调接口 | 触发时机 | 适用场景 |
| --- | --- | --- |
| InferShape | 编译期Shape推导 | 所有场景（可选） |
| InferDataType | 编译期DataType推导 | 所有场景（可选） |
| Compile | 编译期RTC编译kernel | 场景2/场景3 |
| Serialize | ATC编译期写入OM | 场景3 |
| Execute | 运行时执行kernel | 所有场景（必须实现） |
| Deserialize | OM加载时恢复binary | 场景3 |
| UpdateHostArgs | I/O地址变化时刷新 | 静态图下沉场景 |

## 2. 再选定框架

| 前端 | 入图路径 | 转换机制 | 手写REG_OP？ | 关键交付件 | 文档入口 |
| --- | --- | --- | --- | --- | --- |
| PyTorch | FX Graph → GE Graph | TorchAir converter | 是 | converter+GE .so | [PyTorch入图](pytorch_into_graph.md) |
| TensorFlow | TF Graph → GE Graph | Adapter自动映射 | 否（自动生成） | npu_supported_ops.json+GE .so | [TensorFlow入图](tf_into_graph.md) |
| ONNX | ONNX Model → GE Graph | Parser+解析插件 | 是 | libonnx_parser_plugin.so+GE .so | [ONNX入图](onnx_into_graph.md) |
| GE 原生 | 直接API构图 | 无转换层 | 是 | GE .so | [GE原生构图入图](ge_native_into_graph.md) |

各框架下的交付件清单请参见[交付件与OPP规范](deliverables_and_opp.md)。

## 3. 通用5阶段总览

自定义算子入图整体分为5个渐进式阶段，所有框架通用（差异仅在交付件形态），各阶段职责以及涉及交付件如下：

| 阶段 | 职责 | 涉及交付件 |
| --- | --- | --- |
| 阶段1：算子定义 | 声明算子元信息（输入、输出、属性） | proto头文件等 |
| 阶段2：前端接入 | 将前端算子类型映射到GE类型 | 解析插件.so等 |
| 阶段3：图构建入图 | 前端计算图转换为GE计算图 | 无额外交付件（由框架完成） |
| 阶段4：图编译 | 编译期推导和编译 | GE交付件.so中的回调实现 |
| 阶段5：图执行 | 运行时执行kernel | GE交付件.so中的回调实现 |

各阶段与[1. 先定位执行场景](#1-先定位执行场景)的关系，如下表所示：

| 执行场景 | 阶段1-3 | 阶段4 | 阶段5 |
| --- | --- | --- | --- |
| 场景1：动态图在线执行 | 必须完成 | 可跳过 | Execute |
| 场景2：在线编译+执行 | 必须完成 | Compile | Execute |
| 场景3：离线OM下沉 | 必须完成 | Compile+Serialize | Deserialize+Execute |
| 场景4：数据依赖shape | 必须完成 | InferShape（上界） | Execute+shape回写 |

## 4. 交付件速查

各框架完整目录树请参见[交付件与OPP规范](deliverables_and_opp.md)。
