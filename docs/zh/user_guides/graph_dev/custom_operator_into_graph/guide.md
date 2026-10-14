# 导读

自定义算子允许开发者以独立.so交付件的形式，将自有Kernel（如Ascend C、Triton或其他设备侧实现）接入GE图编译与执行流程。该机制无需修改GE框架代码，即可复用图下沉（在线编译、OM离线模型、静态图地址刷新）带来的性能优势。

## 如何开始

| 角色/要做什么 | 涉及章节/阅读建议 |
| --- | --- |
| 决策者：评估该特性值不值得用 | 导读（必读，首次接触自定义算子入图场景时阅读） |
| 开发者：还没装环境，先确认版本配套与前置检查 | [环境准备](env_preparation.md)（必读，首次接触自定义算子入图场景时阅读） |
| 开发者：想10分钟跑通最小示例（推荐第一站） | [快速开始](quick_start.md)（必读，首次接触自定义算子入图场景时阅读） |
| 开发者：理解接口体系、注册机制、执行场景后再动手 | [概念与设计](concept_and_design.md)（按需，涉及对应场景/框架/概念时阅读） |
| 已选定框架，按5阶段流程接入 | [入图流程](into_graph_flow.md)（按需，涉及对应场景/框架/概念时阅读） |
| 需要回调API细节（Execute/InferShape/Compile/Serialize） | 参考/[回调接口详解](callback_interface.md)（专家，实现高级能力（RTC在线编译、OM序列化、静态下沉地址刷新）时阅读） |
| 想知道交付件目录、.so 命名、部署规范 | 参考/[交付件与OPP规范](deliverables_and_opp.md)（按需，涉及对应场景/框架/概念时阅读） |
| 运行失败、精度不对、算子找不到 | 参考/[调试与常见问题](debug_and_common_issues.md)（按需，涉及对应场景/框架/概念时阅读） |
| 遇到不认识的概念 | 参考/[术语表](glossary.md)（按需，涉及对应场景/框架/概念时阅读） |

## 适用场景

- 框架前端（PyTorch/TensorFlow/ONNX/GE 原生）引入了GE未支持的算子时，需要在NPU上执行。
- 接入自定义kernel实现（融合算子、特定优化），复用图下沉能力。
- 以独立.so交付，独立于GE版本随时替换更新。

**使用约束：**

- 不提供kernel本身的实现能力（kernel仍需用Ascend C/Triton等自行开发）。
- 不改变GE已有算子的注册与调度机制，仅提供自定义算子的接入通道。
- 具体接口、API 名称随CANN/GE版本演进，需要以目标版本为准。

## 与GE Ascend IR算子的区别

| 维度 | 自定义算子 | Ascend IR算子 |
| --- | --- | --- |
| 注册方式 | REG_OP+REG_AUTO_MAPPING_OP，以.so交付件加载 | GE内部REG_OP+引擎注册 |
| 编译产物 | 独立.so，通过ASCEND_CUSTOM_OPP_PATH加载 | 随GE发布 |
| 执行方式 | 框架回调用户实现的Execute/Compile | GE引擎调度（DNNEngine等） |
| 更新方式 | 独立于GE，可随时替换.so | 随GE版本更新 |

## 框架支持情况/入图路径对比

| 前端 | 入图路径 | 转换机制 |
| --- | --- | --- |
| PyTorch | FX Graph → GE Graph | TorchAir converter |
| TensorFlow | TF Graph → GE Graph | Adapter自动映射 |
| ONNX | ONNX Model → GE Graph | Parser+解析插件 |
| GE原生 | 直接API构图 | 无转换层，直接API构图 |

## 必备技能

| 技能 | 用途 | 必需程度 |
| --- | --- | --- |
| C++（含 CMake） | 编写GE交付件.so、REG_OP注册 | 必需 |
| Ascend C或acl基础 | 编写/调用 kernel、理解stream/launch | 必需 |
| PyTorch算子注册（TORCH_LIBRARY） | PyTorch场景schema与实现绑定 | PyTorch场景必需 |
| 图编译基本概念（AIR/OM/ATC） | 理解编译期回调（InferShape/Compile/Serialize） | 建议 |
