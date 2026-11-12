# Triton Custom Operator TensorFlow Graph Integration Sample

## Sample Overview

- Graph construction entry: `TensorFlow`
- Operator programming language: `Triton`
- Compilation method: `Pre-compiled to npubin`
- Model sinking capability: `Not applicable`
- Core pipeline: `Triton kernel -> TensorFlow deliverables + GE deliverables -> TensorFlow graph integration execution`
- Difference from other samples: This sample focuses on Triton kernel and TensorFlow/GE deliverables cooperation, not involving TorchAir, nor `ATC offline compilation -> om offline model` model sinking.

This sample demonstrates how to integrate Triton Add kernel into TensorFlow and complete NPU execution through GE custom operator deliverables. The pipeline includes `npubin` generation, TensorFlow custom operator so building, GE deliverables building, and final TensorFlow script result verification.

## Applicable Scenarios

- Want to see custom operator graph integration example in `TensorFlow + Triton` scenario.
- Want to reference cooperation between `npubin`, TensorFlow deliverables, and GE deliverables.
- Want to verify numerical correctness of Triton kernel in TensorFlow graph.
- Not suitable for understanding `TorchAir` or `ATC offline compilation -> om offline model` pipeline.

## Prerequisites

### CANN

- CANN toolkit and ops packages installed and configured.
- `ASCEND_HOME_PATH` set.
- Follow [installation guide](../../../docs/en/build.md#1-environment-preparation) to complete toolkit and ops package installation.

### Framework and Plugins

- Installed `Triton-Ascend`.
- Installed `TensorFlow 1.15` or `TensorFlow 2.6.5` and corresponding framework plugin packages.

References:

- [Triton-Ascend Installation Guide](https://gitcode.com/Ascend/triton-ascend/blob/main/docs/en/installation_guide.md)
- [Triton-Ascend Quick Start](https://gitcode.com/Ascend/triton-ascend/blob/main/docs/en/quick_start.md)
- [TensorFlow 1.15 Migration Guide](https://www.hiascend.com/document/detail/zh/TensorFlowCommunity/850/migration/tfmigr1/tfmigr1_000008.html)
- [TensorFlow 2.6.5 Migration Guide](https://www.hiascend.com/document/detail/zh/TensorFlowCommunity/850/migration/tfmigr2/tfmigr2_000007.html)

### Environment Variables

- `ASCEND_HOME_PATH`
- `ASCEND_CUSTOM_OPP_PATH`
- If need to customize `npubin` output directory, can set `TRITON_CACHE_DIR`

### Additional Dependencies

- `python3`
- `g++`
- `cmake`

## Quick Run

Execute shortest path in `examples/custom_op/triton_add_custom` directory

### Step 0: Check Whether the npubin Enables FFTS Scheduling

Run the following script in the environment where the `npubin` is compiled (that is, the environment where `add_custom_kernel.py` runs successfully, with `torch`, `torch_npu` and Triton-Ascend importable), to check whether the `npubin` compiled by Triton depends on the FFTS scheduling feature:

```bash
python3 - <<'PY'
import torch  # noqa: F401
import torch_npu  # noqa: F401  # npu_utils.so links against libtorch_npu.so, load it first

from triton.backends.ascend.utils import is_ffts_supported, force_disable_ffts

try:
    # Prefer the real compilation target, same source as the triton-ascend launcher check
    from triton.backends.ascend.driver import NPUUtils
    arch = NPUUtils().get_arch()
except Exception as e:
    # Expose the real reason instead of silently falling back to the deprecated helper
    print("NPUUtils failed:", e)
    from triton.backends.ascend.utils import get_ascend_arch_from_env
    arch = get_ascend_arch_from_env()

if not arch:
    print("arch = <empty>: undetermined, do NOT treat it as False")
    raise SystemExit(1)

try:
    disabled = force_disable_ffts(arch)
except TypeError:
    # Compatible with the legacy argument-less signature
    disabled = force_disable_ffts()

print("arch =", arch)
print("need ffts_addr =", is_ffts_supported(arch) and not disabled)
PY
```

- `need ffts_addr = True`: the `npubin` was compiled with the FFTS scheduling feature enabled, and the first argument of kernel args must be the C2C control region address (`ffts_addr`). The sample code does not contain this argument by default, so you **must** modify `ge/src/custom_op.cpp` following [Code Adaptation When FFTS Scheduling Is Enabled](#code-adaptation-when-ffts-scheduling-is-enabled-required-when-step-0-prints-true) and rebuild the GE deliverable. Otherwise the kernel argument layout does not match what the `npubin` expects, and task launching fails or the result check does not pass.
- `need ffts_addr = False`: keep the sample's default layout; no modification is needed.
- The script prints `arch = <empty>: undetermined, do NOT treat it as False` and exits with code `1`: the compilation target is unavailable in the current environment, which must **not** be treated as `False`. Confirm that `torch`, `torch_npu` and Triton-Ascend are correctly installed and that the NPU device is visible (`npu-smi info` works), then rerun the script in the same environment where the `npubin` was generated. If `NPUUtils failed: ...` was printed, use that exception message to locate the environment problem.

This check uses the same logic that triton-ascend applies when generating launchers and when deciding whether to disable FFTS while compiling the `npubin`: `is_ffts_supported(arch) and not force_disable_ffts(arch)`, where `arch` is the compilation target.

> **Note:**
>
> - The script must `import torch_npu` first: the `npu_utils.so` loaded by `NPUUtils()` links against `libtorch_npu.so`, so importing it raises `ImportError` otherwise. If that error were swallowed silently, the script would fall back to the deprecated helper and reach a wrong conclusion.
> - Do not rely on `get_ascend_arch_from_env()` alone for this check. Newer Triton-Ascend versions deprecated that helper (it only prints a warning and returns an empty string), which makes the check always print `False`. The signature of `force_disable_ffts` is either argument-less or takes `arch`, depending on the version; the script above handles both.

### Recommended Method

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

Before execution, confirm `bin_path` in `ge/src/custom_op.cpp` can access `add_kernel.npubin` generated in step 1. If successful, terminal will print `The result of tf and ac is the same.`

### Step-by-step Method

1. First execute `python3 add_custom_kernel.py` in `add_custom_kernel/` to generate `npubin`.
2. Then execute `bash build_tf.sh` in `tensorflow/` to generate `outputs/libcustom_ops.so`.
3. Next execute `bash run.sh` in `ge/` to generate `build_out/libcust_opapi.so` and `framework/tensorflow/npu_supported_ops.json`. If step 0 prints `need ffts_addr = True`, complete the source adaptation described below before this step.
4. Execute `export ASCEND_CUSTOM_OPP_PATH="$(pwd)/build_out:$ASCEND_CUSTOM_OPP_PATH"` to add `libcust_opapi.so` directory to environment variables.
5. Finally return to `tensorflow/` directory and execute `python3 ../script/run_add_custom_tf_1.15.py`.

If need to adjust `npubin` output location, can first set `TRITON_CACHE_DIR`; if need to re-specify `npubin` path, can modify `bin_path` in `ge/src/custom_op.cpp` according to actual cache location.

### Code Adaptation When FFTS Scheduling Is Enabled (Required When Step 0 Prints True)

The sample code targets an `npubin` compiled without the FFTS scheduling feature. If step 0 prints `need ffts_addr = True`, modify `ge/src/custom_op.cpp` before running `ge/run.sh`, to add `ffts_addr` as the first kernel argument in 3 places. `ffts_addr` is the C2C control region address obtained through `aclrtGetHardwareSyncAddr`, which is already declared by `acl/acl_rt.h` included at the top of the file, so no extra header is needed. GE uses the same API to obtain this address when launching tasks.

**Change 1**: after the args assembly comment block (`// 拼装 args`) and before `struct __attribute__((packed)) {`, add the code that obtains the C2C control region address.

```cpp
    void *ffts_addr = nullptr;
    if (aclrtGetHardwareSyncAddr(&ffts_addr) != ACL_ERROR_NONE) {
      std::cerr << __FILE__ << ":" << __LINE__ << " get ffts addr failed" << std::endl;
      return GRAPH_FAILED;
    }
```

**Change 2**: add the `ffts_addr` field at the very beginning of the member list of `struct __attribute__((packed))`.

```cpp
      void *ffts_addr __attribute__((aligned(8)));
```

**Change 3**: pass `ffts_addr` as the first item of the `args` initializer list.

```cpp
        ffts_addr,
```

After the modification, the `args` assembly part should look like the following snippet, which can be used for comparison (the layout matches the repository `.clang-format`):

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

After modifying the source code, run `bash run.sh` again to regenerate `libcust_opapi.so`, then continue with the remaining steps. If this adaptation is skipped, `aclrtLaunchKernelWithHostArgs` typically returns a value other than `ACL_ERROR_NONE` (the terminal prints `aclError: <error code>`), or the result check fails because the arguments are shifted.

`aclrtGetHardwareSyncAddr` is supported by Atlas A3 and Atlas A2 products, and is not supported by Ascend 950PR/950DT, Atlas 200I/500 A2 and some other products. For details, see the "Kernel Loading and Execution" section of the [Runtime API reference](https://gitcode.com/cann/runtime/blob/master/docs/zh/api_ref/README.md), which is currently available in Chinese only.

## Directory Structure and Key Files

```text
triton_add_custom
├── README.md
├── add_custom_kernel
│   └── add_custom_kernel.py        // Triton Add kernel implementation and compilation entry
├── ge
│   ├── CMakeLists.txt              // GE deliverables build script
│   ├── gen_npu_supported_ops_json.sh
│   ├── run.sh
│   └── src
│       └── custom_op.cpp           // Triton operator GE deliverables
├── tensorflow
│   ├── add_custom_triton_tf.cc     // TensorFlow-side custom operator declaration
│   └── build_tf.sh                 // TensorFlow deliverables build script
├── script
│   └── run_add_custom_tf_1.15.py   // TensorFlow test script
├── static_shape.png
└── dynamic_shape.png
```

Key files:

- `add_custom_kernel/add_custom_kernel.py`
  Triton Add kernel implementation and compilation entry.
- `tensorflow/add_custom_triton_tf.cc`
  TensorFlow-side custom operator declaration.
- `tensorflow/build_tf.sh`
  Compile `libcustom_ops.so`.
- `ge/src/custom_op.cpp`
  GE custom operator deliverables, implement loading `npubin` and initiating kernel execution.
- `ge/CMakeLists.txt`
  Build `libcust_opapi.so` and generate `npu_supported_ops.json`.
- `ge/run.sh`
  One-click script for GE substeps, responsible for configure, build, and install.
- `script/run_add_custom_tf_1.15.py`
  TensorFlow graph execution script, responsible for numerical consistency verification.

## Core Pipeline

1. `add_custom_kernel/add_custom_kernel.py` generates Triton kernel corresponding `npubin`.
2. `tensorflow/build_tf.sh` builds TensorFlow-side `libcustom_ops.so`.
3. `ge/CMakeLists.txt` builds GE deliverable `libcust_opapi.so`, and generates `npu_supported_ops.json` for TensorFlow use.
4. `script/run_add_custom_tf_1.15.py` loads TensorFlow-side so, and executes custom AddCustom operator through TensorFlow graph.

## Build Artifacts

- `tensorflow/outputs/libcustom_ops.so`
  TensorFlow-side custom operator deliverable.
- `ge/build_out/libcust_opapi.so`
  GE-side custom operator deliverable.
- `ge/build_out/framework/tensorflow/npu_supported_ops.json`
  Description file used by TensorFlow Adapter when loading custom operator support information.
- `add_kernel.npubin`
  Triton kernel compiled binary file, default written by Triton to cache directory, path can be controlled through `TRITON_CACHE_DIR`.

## Result Verification

When successful, can observe:

- `tensorflow/outputs/libcustom_ops.so` generated.
- `ge/build_out/libcust_opapi.so` generated.
- `ge/build_out/framework/tensorflow/npu_supported_ops.json` generated.
- Terminal output includes `The result of tf and ac is the same.`.

If failed, prioritize checking:

- When step 0 prints `need ffts_addr = True`, whether `ge/src/custom_op.cpp` has been modified following "Code Adaptation When FFTS Scheduling Is Enabled", and whether `ge/run.sh` has been executed again to regenerate `libcust_opapi.so`.
- Whether `bin_path` in `ge/src/custom_op.cpp` can correctly access generated `npubin`.
- Whether `ASCEND_CUSTOM_OPP_PATH` already includes `ge/build_out`.
- Whether `./outputs/libcustom_ops.so` can be accessed under test script startup directory.
- Whether TensorFlow, framework plugin package and Triton-Ascend installation match.

## Notes / Limitations

- Current sample depends on Triton-Ascend target hardware support scope, please refer to [Triton-Ascend project description](https://gitcode.com/Ascend/triton-ascend#hardware-support).
- Whether `ffts_addr` is required depends on whether the FFTS scheduling feature was enabled when the `npubin` was compiled, not on the device model. After regenerating the `npubin` (changing the device, upgrading Triton-Ascend, modifying `TRITON_CACHE_DIR` and so on), run step 0 again, and if necessary redo the source adaptation and rebuild the GE deliverable.
- `bin_path` in `ge/src/custom_op.cpp` needs to be consistent with actual generated `npubin` path.
- Test script defaults to loading TensorFlow-side so from `./outputs/libcustom_ops.so` in current working directory, recommend running test script in `tensorflow/` directory.
- Sample defaults to using static shape; if need dynamic shape, can enable `compile_dynamic_mode` configuration in test script.

## Appendix

### Dynamic / Static Shape Debug Switch

In `script/run_add_custom_tf_1.15.py` can enable dynamic shape through following configuration:

```python
custom_op.parameter_map["compile_dynamic_mode"].b = True
```

### Static Shape Dump Data Configuration

In `script/run_add_custom_tf_1.15.py` can enable dump data capability through following configuration:

```python
custom_op.parameter_map["enable_dump"].b = True
custom_op.parameter_map["dump_mode"].s = tf.compat.as_bytes("all")
custom_op.parameter_map["dump_path"].s = tf.compat.as_bytes("/home/test/output")
```

### Profiling Switch

```python
custom_op.parameter_map["profiling_mode"].b = True
custom_op.parameter_map["profiling_options"].s = tf.compat.as_bytes(
    '{"output":".","training_trace":"on","task_trace":"on","hccl":"on","aicpu":"on","aic_metrics":"PipeUtilization","msproftx":"off"}'
)
```

After exporting profiling data, can use following command to view result directory:

```bash
msprof --export=on --output=${profiling_path}
cd ${profiling_path}/mindstudio_profiler_output
```

Then find `msprof_*.json` and use tools like `chrome://tracing/` to load.

### Profiling Phenomenon Description

- Static shape result illustration: ![static_shape.png](./static_shape.png)
- Dynamic shape result illustration: ![dynamic_shape.png](./dynamic_shape.png)

Under static shape can observe `MODEL_EXECUTE`, `EVENT_WAIT`, `MEMCPY_ASYNC` and other stream synchronization and data transfer processes; under dynamic shape these phenomena will be different.
