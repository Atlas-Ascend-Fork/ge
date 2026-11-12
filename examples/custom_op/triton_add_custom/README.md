# Triton 自定义算子通过 TensorFlow 入图样例

## 样例概述

- 构图入口：`TensorFlow`
- 算子编程语言：`Triton`
- 编译方式：`预编译为 npubin`
- 模型下沉能力：`不涉及`
- 核心链路：`Triton kernel -> TensorFlow 交付件 + GE 交付件 -> TensorFlow 入图执行`
- 与其他 sample 的区别：本样例聚焦 Triton kernel 与 TensorFlow/GE 交付件协同，不涉及 TorchAir，也不涉及 `ATC离线编译 -> om离线模型` 模型下沉。

本样例展示如何把 Triton Add kernel 接入 TensorFlow，并通过 GE 自定义算子交付件完成 NPU 执行。链路包含 `npubin` 生成、TensorFlow 自定义算子 so 构建、GE 交付件构建，以及最终的 TensorFlow 脚本结果校验。

## 适用场景

- 想看 `TensorFlow + Triton` 场景下的自定义算子入图示例。
- 想参考 `npubin`、TensorFlow 交付件和 GE 交付件之间的配合方式。
- 想验证 Triton kernel 在 TensorFlow 图中的数值正确性。
- 不适合用于了解 `TorchAir` 或 `ATC离线编译 -> om离线模型` 链路。

## 前置依赖

### CANN

- 已安装并配置 CANN toolkit 和 ops 包。
- `ASCEND_HOME_PATH` 已设置。
- 参考 [安装指导](../../../docs/zh/build.md#1-环境准备) 完成 toolkit 和 ops 包安装。

### 框架与插件

- 已安装 `Triton-Ascend`。
- 已安装 `TensorFlow 1.15` 或 `TensorFlow 2.6.5` 及其对应框架插件包。

参考：

- [Triton-Ascend 安装说明](https://gitcode.com/Ascend/triton-ascend/blob/main/docs/zh/installation_guide.md)
- [Triton-Ascend 快速开始](https://gitcode.com/Ascend/triton-ascend/blob/main/docs/zh/quick_start.md)
- [TensorFlow 1.15 迁移说明](https://www.hiascend.com/document/detail/zh/TensorFlowCommunity/850/migration/tfmigr1/tfmigr1_000008.html)
- [TensorFlow 2.6.5 迁移说明](https://www.hiascend.com/document/detail/zh/TensorFlowCommunity/850/migration/tfmigr2/tfmigr2_000007.html)

### 环境变量

- `ASCEND_HOME_PATH`
- `ASCEND_CUSTOM_OPP_PATH`
- 如需自定义 `npubin` 输出目录，可设置 `TRITON_CACHE_DIR`

### 额外依赖

- `python3`
- `g++`
- `cmake`

## 快速运行

在 `examples/custom_op/triton_add_custom` 目录下执行以下最短路径

### 步骤 0：判定 npubin 是否启用 FFTS 调度

请在编译 `npubin` 的环境中（即能成功执行 `add_custom_kernel.py` 的环境，需可导入 `torch`、`torch_npu` 与 Triton-Ascend）运行以下脚本，判断 Triton 编译出的 `npubin` 是否依赖 FFTS 调度特性：

```bash
python3 - <<'PY'
import torch  # noqa: F401
import torch_npu  # noqa: F401  # npu_utils.so 链接 libtorch_npu.so，必须先加载

from triton.backends.ascend.utils import is_ffts_supported, force_disable_ffts

try:
    # 优先取真实编译目标，与 triton-ascend 生成 launcher 时的判据一致
    from triton.backends.ascend.driver import NPUUtils
    arch = NPUUtils().get_arch()
except Exception as e:
    # 暂露真实原因，不静默退回已废弃接口
    print("NPUUtils failed:", e)
    from triton.backends.ascend.utils import get_ascend_arch_from_env
    arch = get_ascend_arch_from_env()

if not arch:
    print("arch = <empty>: 无法判定，请勿按 False 处理")
    raise SystemExit(1)

try:
    disabled = force_disable_ffts(arch)
except TypeError:
    # 兼容旧版无参签名
    disabled = force_disable_ffts()

print("arch =", arch)
print("need ffts_addr =", is_ffts_supported(arch) and not disabled)
PY
```

- 输出 `need ffts_addr = True`：`npubin` 启用了 FFTS 调度特性，kernel args 的首参数必须是 C2C 控制区地址（`ffts_addr`）。样例默认代码不含该参数，**必须**先按下文[启用 FFTS 调度时的代码适配](#启用-ffts-调度时的代码适配步骤-0-输出-true-时必做)修改 `ge/src/custom_op.cpp` 并重新编译 GE 交付件；否则 kernel 参数布局与 `npubin` 期望不一致，会出现任务下发失败或结果校验不通过。
- 输出 `need ffts_addr = False`：保持样例默认写法，无需任何修改。
- 脚本打印 `arch = <empty>: 无法判定，请勿按 False 处理` 并以退出码 `1` 结束：当前环境取不到编译目标，**不能**当作 `False` 处理。请先确认 `torch`、`torch_npu`、Triton-Ascend 已正确安装且 NPU 设备可见（`npu-smi info` 能正常输出），再在生成 `npubin` 的同一环境中重跑脚本；若打印了 `NPUUtils failed: ...`，请根据其中的异常信息定位环境问题。

判定逻辑与 triton-ascend 生成 launcher、以及编译 `npubin` 时决定是否关闭 FFTS 的逻辑一致，均为 `is_ffts_supported(arch) and not force_disable_ffts(arch)`，其中 `arch` 取自编译目标。

> **说明：**
>
> - 脚本必须先 `import torch_npu`：`NPUUtils()` 加载的 `npu_utils.so` 链接了 `libtorch_npu.so`，未先导入时会抛 `ImportError`；若该异常被静默吞掉，就会退回已废弃接口并得出错误结论。
> - 请勿用 `get_ascend_arch_from_env()` 单独做该判定。较新版本的 Triton-Ascend 已废弃该接口（仅打印告警并返回空字符串），会使判定结果恒为 `False`；`force_disable_ffts` 在不同版本分别为无参和带 `arch` 参数两种签名，上述脚本已做兼容。

### 推荐方式

```bash
cd add_custom_kernel
python3 add_custom_kernel.py

cd ../tensorflow
bash build_tf.sh

cd ../ge
bash run.sh
export ASCEND_CUSTOM_OPP_PATH="$(pwd)/build_out:$ASCEND_CUSTOM_OPP_PATH"

cd ../tensorflow
python3 ../script/run_add_custom_tf_1.15.py
```

执行前请确认 `ge/src/custom_op.cpp` 中的 `bin_path` 能访问步骤 1 生成的 `add_kernel.npubin`。若运行成功，终端会打印 `The result of tf and ac is the same.`。

### 分步方式

1. 先在 `add_custom_kernel/` 下执行 `python3 add_custom_kernel.py` 生成 `npubin`。
2. 再在 `tensorflow/` 下执行 `bash build_tf.sh` 生成 `outputs/libcustom_ops.so`。
3. 接着在 `ge/` 下执行 `bash run.sh`，生成 `build_out/libcust_opapi.so` 和 `framework/tensorflow/npu_supported_ops.json`。若步骤 0 输出 `need ffts_addr = True`，请先完成下文的源码适配再执行本步骤。
4. 执行 `export ASCEND_CUSTOM_OPP_PATH="$(pwd)/build_out:$ASCEND_CUSTOM_OPP_PATH"`，将 `libcust_opapi.so` 所在目录加入环境变量。
5. 最后回到 `tensorflow/` 目录执行 `python3 ../script/run_add_custom_tf_1.15.py`。

若需要调整 `npubin` 输出位置，可先设置 `TRITON_CACHE_DIR`；若需要重新指定 `npubin` 路径，可按实际缓存位置修改 `ge/src/custom_op.cpp` 中的 `bin_path`。

### 启用 FFTS 调度时的代码适配（步骤 0 输出 True 时必做）

样例默认代码面向未启用 FFTS 调度的 `npubin`。若步骤 0 输出 `need ffts_addr = True`，需在执行 `ge/run.sh` 之前修改 `ge/src/custom_op.cpp`，为 kernel args 增加首参数 `ffts_addr`，共 3 处。`ffts_addr` 为 C2C 控制区地址，通过 `aclrtGetHardwareSyncAddr` 获取，该接口已由文件顶部包含的 `acl/acl_rt.h` 声明，无需新增头文件；GE 下发任务时同样使用该接口获取此地址。

**修改点 1**：在 `// 拼装 args` 注释之后、`struct __attribute__((packed)) {` 之前，新增获取 C2C 控制区地址的代码。

```cpp
    void *ffts_addr = nullptr;
    if (aclrtGetHardwareSyncAddr(&ffts_addr) != ACL_ERROR_NONE) {
      std::cerr << __FILE__ << ":" << __LINE__ << " get ffts addr failed" << std::endl;
      return GRAPH_FAILED;
    }
```

**修改点 2**：在 `struct __attribute__((packed))` 的成员列表最前面新增 `ffts_addr` 字段。

```cpp
      void *ffts_addr __attribute__((aligned(8)));
```

**修改点 3**：在 `args` 初始化列表最前面传入 `ffts_addr`。

```cpp
        ffts_addr,
```

修改完成后，`args` 拼装部分应如下所示，可直接比对（排版符合仓库 `.clang-format`）：

```cpp
    void *ffts_addr = nullptr;
    if (aclrtGetHardwareSyncAddr(&ffts_addr) != ACL_ERROR_NONE) {
      std::cerr << __FILE__ << ":" << __LINE__ << " get ffts addr failed" << std::endl;
      return GRAPH_FAILED;
    }
    struct __attribute__((packed)) {
      void *ffts_addr __attribute__((aligned(8)));
      void *sync_block_lock __attribute__((aligned(8)));
      void *workspace_addr __attribute__((aligned(8)));
      const void *arg0 __attribute__((aligned(8)));
      const void *arg1 __attribute__((aligned(8)));
      void *arg2 __attribute__((aligned(8)));
      int32_t arg3 __attribute__((aligned(4)));
      int32_t grid_x __attribute__((aligned(4)));
      int32_t grid_y __attribute__((aligned(4)));
      int32_t grid_z __attribute__((aligned(4)));
    } args = {
        ffts_addr, nullptr, nullptr, input_x->GetAddr(), input_y->GetAddr(), z_addr, static_cast<int32_t>(n_elements),
        grid_x, grid_y, grid_z,
    };
```

源码改完后必须重新执行 `bash run.sh` 生成 `libcust_opapi.so`，再继续后续步骤。若跳过本节修改直接运行，典型现象是 `aclrtLaunchKernelWithHostArgs` 返回非 `ACL_ERROR_NONE`（终端打印 `aclError: <错误码>`），或因参数错位导致结果校验不通过。

`aclrtGetHardwareSyncAddr` 的产品支持范围为 Atlas A3 系列和 Atlas A2 系列，Ascend 950PR/950DT、Atlas 200I/500 A2 等不支持，接口说明请参见《[Runtime运行时API](https://gitcode.com/cann/runtime/blob/master/docs/zh/api_ref/README.md)》中的“Kernel加载与执行”。

## 目录结构与关键文件

```text
triton_add_custom
├── README.md
├── add_custom_kernel
│   └── add_custom_kernel.py        // Triton Add kernel 实现与编译入口
├── ge
│   ├── CMakeLists.txt              // GE 交付件构建脚本
│   ├── gen_npu_supported_ops_json.sh
│   ├── run.sh
│   └── src
│       └── custom_op.cpp           // Triton 算子入 GE 交付件
├── tensorflow
│   ├── add_custom_triton_tf.cc     // TensorFlow 侧自定义算子声明
│   └── build_tf.sh                 // TensorFlow 交付件构建脚本
├── script
│   └── run_add_custom_tf_1.15.py   // TensorFlow 测试脚本
├── static_shape.png
└── dynamic_shape.png
```

重点文件：

- `add_custom_kernel/add_custom_kernel.py`
  Triton Add kernel 的实现与编译入口。
- `tensorflow/add_custom_triton_tf.cc`
  TensorFlow 侧自定义算子声明。
- `tensorflow/build_tf.sh`
  编译 `libcustom_ops.so`。
- `ge/src/custom_op.cpp`
  GE 自定义算子交付件，实现加载 `npubin` 并发起 kernel 执行。
- `ge/CMakeLists.txt`
  构建 `libcust_opapi.so` 并生成 `npu_supported_ops.json`。
- `ge/run.sh`
  GE 子步骤的一键脚本，负责 configure、build 和 install。
- `script/run_add_custom_tf_1.15.py`
  TensorFlow 图执行脚本，负责数值一致性校验。

## 核心链路

1. `add_custom_kernel/add_custom_kernel.py` 负责生成 Triton kernel 对应的 `npubin`。
2. `tensorflow/build_tf.sh` 构建 TensorFlow 侧 `libcustom_ops.so`。
3. `ge/CMakeLists.txt` 构建 GE 交付件 `libcust_opapi.so`，并生成 `npu_supported_ops.json` 供TensorFlow使用。
4. `script/run_add_custom_tf_1.15.py` 加载 TensorFlow 侧 so，并通过 TensorFlow 图执行自定义 AddCustom 算子。

## 构建产物

- `tensorflow/outputs/libcustom_ops.so`
  TensorFlow 侧自定义算子交付件。
- `ge/build_out/libcust_opapi.so`
  GE 侧自定义算子交付件。
- `ge/build_out/framework/tensorflow/npu_supported_ops.json`
  TensorFlow Adapter 加载自定义算子支持信息时使用的描述文件。
- `add_kernel.npubin`
  Triton kernel 编译后的二进制文件，默认由 Triton 写入缓存目录，路径可通过 `TRITON_CACHE_DIR` 控制。

## 结果校验

成功时可观察到：

- `tensorflow/outputs/libcustom_ops.so` 已生成。
- `ge/build_out/libcust_opapi.so` 已生成。
- `ge/build_out/framework/tensorflow/npu_supported_ops.json` 已生成。
- 终端输出包含 `The result of tf and ac is the same.`。

若失败，优先检查：

- 步骤 0 输出 `need ffts_addr = True` 时，是否已按《启用 FFTS 调度时的代码适配》修改 `ge/src/custom_op.cpp`，并重新执行 `ge/run.sh` 生成 `libcust_opapi.so`。
- `ge/src/custom_op.cpp` 中的 `bin_path` 是否能正确访问生成的 `npubin`。
- `ASCEND_CUSTOM_OPP_PATH` 是否已包含 `ge/build_out`。
- 测试脚本启动目录下是否能访问 `./outputs/libcustom_ops.so`。
- TensorFlow、框架插件包和 Triton-Ascend 安装是否匹配。

## 注意事项 / 限制

- 当前样例依赖 Triton-Ascend 对目标硬件的支持范围，请参考 [Triton-Ascend 项目说明](https://gitcode.com/Ascend/triton-ascend#硬件支持)。
- 是否需要 `ffts_addr` 取决于 `npubin` 编译时是否启用 FFTS 调度特性，而非设备型号；重新生成 `npubin`（更换设备、升级 Triton-Ascend、修改 `TRITON_CACHE_DIR` 等）后需重新执行步骤 0 判定，必要时重新适配源码并重编 GE 交付件。
- `ge/src/custom_op.cpp` 中的 `bin_path` 需要与实际生成的 `npubin` 路径保持一致。
- 测试脚本默认从当前工作目录的 `./outputs/libcustom_ops.so` 加载 TensorFlow 侧 so，建议在 `tensorflow/` 目录下运行测试脚本。
- 样例默认使用静态 shape；如需动态 shape，可在测试脚本中打开 `compile_dynamic_mode` 配置。

## 附录

### 动态 / 静态 shape 调测开关

在 `script/run_add_custom_tf_1.15.py` 中可通过以下配置启用动态 shape：

```python
custom_op.parameter_map["compile_dynamic_mode"].b = True
```

### 静态 shape dump data配置

在 `script/run_add_custom_tf_1.15.py` 中可通过以下配置启用dump data能力：

```python
custom_op.parameter_map["enable_dump"].b = True
custom_op.parameter_map["dump_mode"].s = tf.compat.as_bytes("all")
custom_op.parameter_map["dump_path"].s = tf.compat.as_bytes("/home/test/output")
```

### Profiling 开关

```python
custom_op.parameter_map["profiling_mode"].b = True
custom_op.parameter_map["profiling_options"].s = tf.compat.as_bytes(
    '{"output":".","training_trace":"on","task_trace":"on","hccl":"on","aicpu":"on","aic_metrics":"PipeUtilization","msproftx":"off"}'
)
```

导出 profiling 数据后，可使用如下命令查看结果目录：

```bash
msprof --export=on --output=${profiling_path}
cd ${profiling_path}/mindstudio_profiler_output
```

然后找到 `msprof_*.json` 并使用如 `chrome://tracing/` 的工具加载。

### Profiling 现象说明

- 静态 shape 结果示意：![static_shape.png](./static_shape.png)
- 动态 shape 结果示意：![dynamic_shape.png](./dynamic_shape.png)

静态 shape 下可观测到 `MODEL_EXECUTE`、`EVENT_WAIT`、`MEMCPY_ASYNC` 等流同步和数据传输过程；动态 shape 下这些现象会有所不同。
