# Value Dependent Shape 自定义算子(值依赖算子)样例

## 样例概述

- 构图入口：`GE`
- 算子编程语言：`Ascend C`
- 编译方式：`.asc` 与 `.cpp` 同 target 编译
- 模型下沉能力：`不涉及`
- 核心链路：`Ascend C kernel 与 host 侧 custom op 同库编译 -> GE 交付件 -> 进程内构图 -> Session::RunGraph -> RT2 动态执行`
- 与其他 sample 的区别：本样例聚焦值依赖算子的标准实现：通过 `IMPL_OP` 的 `InputsDataDependency` 声明数据依赖输入，在 `InferShape` 阶段读取 `concat_dim` 输入的值确定拼接轴并推导输出 shape。

本样例展示如何开发一个值依赖自定义算子，并将算子接入 GE 图执行。样例以 ConcatV2 拼接算子为例：交付件侧通过 `InferShape` 读取输入值推导输出 shape、`EagerExecuteOp` 完成执行，进程内构图后直接调用 `Session::RunGraph`。

`ConcatV2Custom` 的语义对齐 ops-math 生产算子 ConcatV2（TensorFlow 兼容）：动态输入 `x` 为至少 2 个同 dtype 张量，输入 `concat_dim` 为 `int32/int64` 标量，输出 `y` 为各输入沿 `concat_dim` 指定轴拼接的结果。由于拼接轴（进而输出 shape 的哪一个维被求和）取决于 `concat_dim` 输入的**数值**而非其 shape，本算子属于值依赖算子。

## 适用场景

- 想了解如何开发一个自定义值依赖算子（输出 shape 依赖输入 value）并通过 GE 入图执行。
- 想了解值依赖算子的执行流程：`InputsDataDependency` 数据依赖声明、编译期/运行期 `InferShape` 读值推导、`Execute` 按预填 shape 执行。

## 前置依赖

### CANN

- 已正确安装并配置 CANN 环境，例如执行过 `source ${ASCEND_HOME_PATH}/set_env.sh`。
- 当前环境具备 `ACL`、`GE`、`Graph`、`Ascend C` 相关头文件与库。
- 参考 [安装指导](../../../docs/zh/quick_install.md) 完成 toolkit 和 ops 包安装。

### 框架与插件

- 本样例不依赖 PyTorch、TensorFlow 或 TorchAir。

### 环境变量

- `ASCEND_HOME_PATH`
- `ASCEND_CUSTOM_OPP_PATH` 会在 `run.sh` 中自动追加为当前 sample 的 `output/`

### 额外依赖

- `cmake`
- `g++`

## 快速运行

在 `examples/custom_op/value_dependent_shape_custom` 目录下执行：

### 推荐方式

```bash
source ${ASCEND_HOME_PATH}/set_env.sh
bash run.sh
```

`run.sh` 会自动完成 configure、build、install，并把 `output/` 追加到 `ASCEND_CUSTOM_OPP_PATH`。运行成功时终端会打印（`[InferShape]` 行为值依赖推导链路的直接证据）：

```text
=== concat_dim as Data input (axis value read at execution time) ===
[InferShape] concat_dim value unavailable, degrade to unknown dims     ← 编译期（值不可得，保秩退化）
[InferShape] got concat axis from input value: 1 (normalized to 1)    ← 运行期：执行图 InferShape 节点读框架物化的 host 副本重推
[Execute] use pre-filled output shape: [2, 3, 2]                      ← Execute 取预填 shape，无需读轴值
output shape: [2, 3, 2]
output values: 1 2 3 4 9 10 5 6 7 8 11 12
```

机制说明：

- **Data 输入**：编译期图输入的值尚不可得（`GetData` 返回空指针），与生产算子一致按保秩语义退化为 `[unknown, unknown, unknown]`；运行期（RunGraph）框架在执行图中、算子执行前串联 InferShape 节点：数据依赖输入已由 lowering 自动物化 host 副本（执行图内 D2H 拷贝节点），节点读到轴值 `1` 重推出精确 shape `[2, 3, 2]`，并由 `ExecuteCustomOpWithInferShape` 预填进输出 tensor；`Execute` 取预填 shape 校验后按精确 shape 申请输出——**全程算子侧不做任何 D2H / stream 同步**。

### 分步方式

```bash
source ${ASCEND_HOME_PATH}/set_env.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
cmake --install build
export ASCEND_CUSTOM_OPP_PATH="$(pwd)/output:$ASCEND_CUSTOM_OPP_PATH"

cd build
./value_dependent_shape_custom_session_run
cd ..
```

其中 `export ASCEND_CUSTOM_OPP_PATH="$(pwd)/output:$ASCEND_CUSTOM_OPP_PATH"` 用于将自定义算子包根目录加入环境变量，随后 GE 会按 `output/op_graph/lib/<os>/<arch>/libconcat_v2_custom_op.so` 规则加载交付件。

## 目录结构与关键文件

```text
value_dependent_shape_custom
├── CMakeLists.txt
├── README.md
├── run.sh
├── ge
│   ├── custom_op.cpp                    // EagerExecuteOp 执行 + IMPL_OP 注册（InferShape + InputsDataDependency）
│   ├── concat_v2_custom.h               // ConcatV2Custom proto 定义
│   └── concat_v2_custom_kernel.asc      // Ascend C kernel 源码
└── session_run
    └── main.cc                          // 进程内构图并直接调用 Session::RunGraph
```

重点文件：

- `ge/custom_op.cpp`
  自定义算子的核心主流程：`InferShape` 实现值依赖 shape 推导（与生产算子 ops-math ConcatV2 的 `InferShape4ConcatV2` 对齐：按 IR 索引 `GetRequiredInputTensor(1)` 读取 `concat_dim` 的值、负轴归一化、动态输入逐实例累加轴维、值不可得时保秩退化），经 `IMPL_OP(ConcatV2Custom).InputsDataDependency({1})` 注册并声明数据依赖输入；`Execute` 不读输入值——取框架预填的输出 shape（运行期 InferShape 重推结果），校验其与输入 shape 满足拼接约束（非轴维一致、轴维=各输入之和）后按精确 shape `MallocOutputTensor` 并下发 kernel。输出 dtype 由 `InferDataType` 显式跟随首个 `x` 输入。
- `ge/concat_v2_custom_kernel.asc`
  device kernel 和 host 侧 launch wrapper 实现，按行块字节拷贝完成 2 路输入沿轴拼接（最小演示实现，未做性能优化）。
- `session_run/main.cc`
  构建最小图并通过 `Session::AddGraph + Session::RunGraph` 执行（`ge.graphRunMode=1` 动态图模式，与生产算子 ConcatV2 GEIR 测试一致）：`concat_dim` 为 host 放置的 Data 输入（编译期退化 unknown、运行期重推精确）。
- `run.sh`
  串起 configure、build、install 和运行过程。

## 核心链路

1. `session_run/main.cc` 构建最小图并通过 `Session::AddGraph + Session::RunGraph` 执行（`ge.graphRunMode=1`，未知 shape 图走 RT2 动态执行链路）：`Data(x0 [2,2,2]) + Data(x1 [2,1,2]) + Data(concat_dim, host 放置) -> ConcatV2Custom`，编译期输出 shape 含 unknown 维。
2. `ge/custom_op.cpp` 中的 `InferShape` 经 `IMPL_OP` 注册，并声明 `concat_dim`（IR 输入 1，`x` 为动态输入占 IR 0）为数据依赖输入。两个取值分支均打印 `[InferShape]` 日志，用于观测链路是否生效：
   - 值不可得（编译期，Data 输入非 const）：`GetData` 返回空指针，与生产算子一致按保秩语义退化为 `[unknown, unknown, unknown]`；
   - 值可得（运行期读框架物化的 host 副本值 1）：输出 = 首输入 shape `[2,2,2]` 的轴 1 替换为 `2 + 1 = 3`，即 `[2,3,2]`（负轴输入会先按 rank 归一化）。
3. **框架侧机制（值依赖算子无需自行实现）**：数据依赖输入经 lowering 自动物化 host 副本（执行图内插入 D2H 拷贝节点，见 `runtime/v2/lowering/placement/placed_lowering_result.cc` 的 `CreateDataDependentResult`）；注册 `ShapeInferOp` 的算子在执行图中被串联运行期 InferShape 节点，`ExecuteCustomOpWithInferShape` 在 Execute 前将重推 shape 预填进输出 tensor（见 `runtime/v2/engine/custom/` 的 `custom_node_converter.cc` / `custom_op_kernel.cc`）。
4. `CMakeLists.txt` 将 `ge/custom_op.cpp` 和 `ge/concat_v2_custom_kernel.asc` 一起编译到 `libconcat_v2_custom_op.so`。
5. `ge/custom_op.cpp` 在 `Execute` 回调中**不读任何输入值**：取预填的输出 shape，校验其与输入 shape 满足拼接约束（输出与首输入恰在轴维不同、轴维=各输入之和、其余维一致），据此推导拷贝布局，按精确 shape 调用 `ctx->MallocOutputTensor(...)` 申请输出（值依赖算子无需像数据依赖算子那样按上界申请并预留 shape buffer），最后调用 `.asc` 中导出的 launch wrapper 完成 kernel 下发。
6. 执行完成后，`session_run/main.cc` 读取输出 tensor，打印实际 shape 和输出值。


## 构建产物

- `output/op_graph/lib/linux/x86_64/libconcat_v2_custom_op.so`
  Linux x86_64 环境下 GE 使用的自定义算子交付件；aarch64 环境对应 `output/op_graph/lib/linux/aarch64/libconcat_v2_custom_op.so`。
- `output/op_graph/include/concat_v2_custom.h`
  构图侧可直接使用的算子 proto 头文件。
- `build/value_dependent_shape_custom_session_run`
  直接 `Session::RunGraph` 的执行程序。

## 结果校验

成功时可观察到：

- `output/op_graph/lib/<os>/<arch>/libconcat_v2_custom_op.so` 已生成。
- `output/op_graph/include/concat_v2_custom.h` 已生成。
- 输出 `output shape: [2, 3, 2]` 与 `output values: 1 2 3 4 9 10 5 6 7 8 11 12`（`x0` 为 `[2,2,2]` 的 1~8，`x1` 为 `[2,1,2]` 的 9~12，沿轴 1 拼接）。
- **值依赖推导链路生效的直接证据**：编译期打印 `[InferShape] concat_dim value unavailable, degrade to unknown dims`；运行期由执行图 InferShape 节点打印 `[InferShape] got concat axis from input value: 1 (normalized to 1)`；`Execute` 打印 `[Execute] use pre-filled output shape: [2, 3, 2]`（输出 shape 来自框架预填，算子侧无 D2H）。
- 可选：`export DUMP_GE_GRAPH=2` 后重跑，在 dump 图（如 `ge_onnx_*_AfterInfershape.pbtxt`）中确认编译期输出 shape 为 `[-1, -1, -1]`（运行期重推后精确）。

若失败，优先检查：

- `ASCEND_HOME_PATH` 是否已设置并已正确 `source` CANN 环境。
- `ASCEND_CUSTOM_OPP_PATH` 是否已包含当前 sample 的 `output/`。
- `output/op_graph/lib/<os>/<arch>/libconcat_v2_custom_op.so` 和 `output/op_graph/include/concat_v2_custom.h` 是否已生成。
- 当前环境是否具备可用 NPU 和可用 Ascend C 编译环境。
- 若 `Execute` 报 shape 非 fully known：说明运行期 InferShape 未重推（预填 shape 仍为 unknown）。确认 `ge.graphRunMode=1` 已生效、GE 版本具备 `ExecuteCustomOpWithInferShape` 机制（算子需注册 `ShapeInferOp`）。

## 注意事项 / 限制

- 样例当前使用 `x0=[2,2,2]`、`x1=[2,1,2]`（float）与 `concat_dim`（值 `1`）做结果校验，实际输出为 `[2, 3, 2]`。
- 演示 kernel 固定 2 路输入，要求输入 shape 全已知、非轴维一致且为连续 ND 布局（`InferShape` 本身支持任意实例数与 unknown 维）。
- 样例依赖可用 NPU 与 Ascend C 编译环境；运行期重推与预填机制依赖 RT2 动态执行链路（`ge.graphRunMode=1`），环境不支持时 `Execute` 显式报错。
- `.asc` 编译参数当前固定为 `--npu-arch=dav-2201`，如目标芯片不同需调整 `CMakeLists.txt`。

## 附录

### 算子规格

| 项目 | 内容 |
| --- | --- |
| 算子类型 | `ConcatV2Custom` |
| 输入 | `x`（动态输入，至少 2 个实例）、`concat_dim` |
| 输出 | `y` |
| 输入 `x` shape | 各实例同 rank、非轴维一致 |
| 输入 `concat_dim` shape | 标量（`[]` 或 `[1]`），其值即拼接轴（支持负轴） |
| 输出 shape | 首输入 shape 的轴维替换为各输入该维之和 |
| 输入 `x` 数据类型 | `DT_FLOAT` / `DT_FLOAT16` / `DT_INT32` |
| 输入 `concat_dim` 数据类型 | `DT_INT32` / `DT_INT64` |
| 输出数据类型 | 与输入 `x` 相同（由 `InferDataType` 跟随首个 `x` 输入） |
| 格式 | `ND` |
| kernel 名称 | `concat_v2_custom` |
| 属性 | `N`（`x` 实例数，演示图固定为 2） |

### 与生产算子 ConcatV2 的对应关系

| 生产算子（ops-math `conversion/concat_v2`） | 本样例 |
| --- | --- |
| `REG_OP(ConcatV2)`：`DYNAMIC_INPUT(x)` + `INPUT(concat_dim)` + `OUTPUT(y)` + `ATTR(N)` | proto 结构一致（dtype 集合裁剪为演示所需子集） |
| `IMPL_OP_INFERSHAPE(ConcatV2).InferShape(InferShape4ConcatV2).InputsDataDependency({1})` | `IMPL_OP(ConcatV2Custom).InputsDataDependency({1})` + `ShapeInferOp::InferShape` 类能力注册（Eager 自定义算子形态） |
| `GetRequiredInputTensor(1)` 按 IR 索引读轴值，值不可得保秩退化 | 同 |
| 负轴归一化、动态输入逐实例累加轴维 | 同 |
| AICPU kernel 执行期读轴值（host 侧不读值） | 本样例 host 侧同样不读值：kernel 拷贝布局由预填 shape 与输入 shape 推得 |

### 值依赖推导约定

本样例的输出 shape 推导分两个阶段，且各分支均打印 `[InferShape]` 日志用于观测：

- `InferShape` 阶段（编译期 + 运行期均回调同一注册函数，保证推导一致）：
  - 编译期取不到值（`GetData` 返回空指针），打印 `[InferShape] concat_dim value unavailable, degrade to unknown dims` 并保秩退化为 `[unknown, unknown, unknown]`；
  - 运行期执行图 InferShape 节点读到框架物化的 host 副本值 1，重推打印 `[InferShape] got concat axis from input value: 1 (normalized to 1)`；
- `Execute` 阶段：不读任何输入值。`ExecuteCustomOpWithInferShape` 已将重推 shape 预填进输出 tensor，`Execute` 校验预填 shape 与输入 shape 满足拼接约束（输出与首输入恰在轴维不同、轴维=各输入之和、其余维一致），据此推导拷贝布局并按精确 shape `MallocOutputTensor` 申请输出内存。

开发要点（值依赖算子通用）：

- 值依赖输入须经 `InputsDataDependency` 声明才能在 `InferShape` 中取值，否则为未定义行为；编译期非 const 输入 `GetData` 返回空指针属预期，应保秩退化而非报错。
- 动态输入取值与 dtype 必须按 IR 索引（`GetRequiredInputTensor` / `GetRequiredInputDesc`），不能用扁平索引（占位 tensor 的 `GetDataType()` 不可靠）；构图用 `create_dynamic_input_x(n, false)` 保持输入 flat 顺序与 IR 声明顺序一致，否则索引错位。
- 值输入的图输入建议声明 host 放置（`SetPlacement(kPlacementHost)`），避免 H2D/D2H 往返。

对当前样例输入，`x0` 为 `[2,2,2]`（值 1~8），`x1` 为 `[2,1,2]`（值 9~12），`concat_dim` 值为 `1`，实际输出 shape 为 `[2, 3, 2]`，输出数据为 `1 2 3 4 9 10 5 6 7 8 11 12`。
