# Quick Start

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:07:38.985Z pushedAt=2026-10-28T03:30:34.454Z nl=2 -->

This chapter demonstrates how to quickly get started with the automatic fusion feature through two paths, based on the different fusion implementation paths.

> [!NOTE] Note
>
> The GE path supports both static and dynamic shapes, making it suitable for scenarios where input shapes vary. The Inductor path currently supports only static shapes.

## GE Path Enabling Automatic Fusion

### TensorFlow Scenario

This section uses the `Abs+ReLU+Exp` operator fusion as an example to demonstrate how to configure and run a fusion case and how to verify the fusion result. The example uses an input with a shape of [128, 192] and the float16 data type, performs 100 inferences on the NPU, and has built-in NPU Profiling. After execution, a profiling directory is generated, making it easy to view the fusion result and performance data.

#### Prerequisites

See [Enabling AutoFuse](autofuse_enable.md#enabling-autofuse-for-the-ge-path) to complete environment setup, install TensorFlow and the TF Adapter plugin, set environment variables, and enable automatic fusion.

#### Example Code

```python
import glob
import os
import subprocess
import numpy as np
import tensorflow as tf
import npu_bridge

# Define the output directory for profiling data and obtain the absolute path of the profiling folder under the current path
PROFILING_DIR = os.path.abspath("./profiling")
# Build the JSON configuration string for profiling
PROFILING_OPTIONS = (
    '{"output":"%s","training_trace":"on","task_time":"on",'
    '"hccl":"on","aicpu":"on","aic_metrics":"PipeUtilization","msproftx":"off"}'
) % PROFILING_DIR


def configure_npu(session_config):
    """
    Configure the TensorFlow Session to support NPU execution and profiling.
    """
    # Obtain the graph option rewrite rules in the session configuration
    custom_op = session_config.graph_options.rewrite_options.custom_optimizers.add()
    # Specify the custom optimizer name as NpuOptimizer
    custom_op.name = "NpuOptimizer"
    # Set the parameter mapping
    # use_off_line: enable offline mode, which is usually used for precompilation or specific inference scenarios
    custom_op.parameter_map["use_off_line"].b = True
    # graph_run_mode: set the graph run mode, where 0 usually indicates the default or synchronous mode
    custom_op.parameter_map["graph_run_mode"].i = 0
    # profiling_mode: enable profiling mode
    custom_op.parameter_map["profiling_mode"].b = True
    # profiling_options: Pass the JSON configuration string defined above.
    custom_op.parameter_map["profiling_options"].s = tf.compat.as_bytes(PROFILING_OPTIONS)
    return session_config


def get_profile_dirs():
    """
    Obtain the set of all performance analysis directories whose names start with PROF_ in the current profiling directory.
    """
    return set(glob.glob(os.path.join(PROFILING_DIR, "PROF_*")))


def export_new_profiling(profile_dirs_before):
    """
    Export the newly added performance analysis data.
    Compare the current directory with the previous directory list to find the newly added analysis directory, and call the msprof tool to export it.
    """
    # Obtain all current PROF_ directories, subtract the previous directories, and get the list of newly added directories.
    new_profile_dirs = sorted(get_profile_dirs() - profile_dirs_before)
    for profile_dir in new_profile_dirs:
        # Call the msprof tool, enable the export function, and specify the output path.
        subprocess.run(
            ["msprof", "--export=on", "--output={}".format(profile_dir)],
            check=True,
        )
    return new_profile_dirs

# ---Define the computation graph---
# Use TensorFlow 1.x placeholders to define the input tensors.
input_tensor = tf.placeholder(tf.float16, shape=[128, 192], name="input")
# Take the absolute value of the input, named "abs".
abs_result = tf.abs(input_tensor, name="abs")
# Apply the ReLU activation function to the absolute value result, named "relu".
relu_result = tf.nn.relu(abs_result, name="relu")
# Apply the exponential function to the ReLU result, named "exp".
output_tensor = tf.exp(relu_result, name="exp")

# ---Prepare test data---
# Set a random seed to ensure reproducible results.
np.random.seed(0)
# Generate random data.
input_data = np.random.uniform(-1.0, 1.0, size=(128, 192)).astype(np.float16)

# ---Run the training/inference loop---
# Before enabling Profiling, record the existing profiling directories for later comparison to identify newly generated data.
profile_dirs_before = get_profile_dirs()
# Configure the TensorFlow Session.
session_config = tf.ConfigProto(allow_soft_placement=True, log_device_placement=False)
# Apply the NPU configuration (enable profiling, etc.).
configure_npu(session_config)

# Create a Session and execute the computation.
with tf.Session(config=session_config) as session:
    for _ in range(100):
        session.run(output_tensor, feed_dict={input_tensor: input_data})

# After the loop ends, export the newly generated profiling data.
export_new_profiling(profile_dirs_before)
```

#### Verify Fusion Result

After execution, a profiling directory is generated in the current directory. The op_summary_*.csv file exported by Profiling is usually located in the following path:

```tree
profiling/
└── PROF_timestamp_xxx/
    └── mindstudio_profiler_output/
        └── op_summary_timestamp.csv
```

Open the `op_summary_*.csv` file corresponding to this run and view the operator list in it. If a fusion kernel whose name starts with `autofuse_` appears, the related operators have been fused. In this example, the fusion kernel is named `autofuse_pointwise_0_Abs_Relu_Exp`. The specific kernel name may vary with the version, so determine it based on the operator type and execution record.

#### Performance Comparison Before and After Fusion

To evaluate the performance gain of AutoFuse, collect Profiling data in the following two scenarios:

1. **AutoFuse enabled**: Configure the environment variable `AUTOFUSE_FLAGS="--enable_autofuse=true"`.
2. **AutoFuse disabled**: Configure the environment variable `AUTOFUSE_FLAGS="--enable_autofuse=false"` as the control group for the unfused scenario.

Note: When AutoFuse is disabled, GE's own graph optimization may still fuse some operators, so independent Abs, Relu, and Exp operators may not necessarily appear in the control scenario. Therefore, whether a fused kernel with the `autofuse_` prefix appears should be used as the criterion for determining whether AutoFuse takes effect.

The two scenarios should use the same input data, number of executions, and Profiling configuration. Compare the execution time within the same computation range, and distinguish the first-time graph compilation overhead from the stable execution time after warm-up. For operators with a high proportion of input and output data transfer, you can further focus on `aiv_mte2_time` and `aiv_mte3_time` in the Profiling data.

For details about how to use the Profiling performance analysis tool, see [Performance Tuning Tool](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/devaids/Profiling/atlasprofiling_16_0001.html).

### TorchAir Scenario

This section describes how to enable the AutoFuse automatic operator fusion feature based on the GE path (PyTorch framework). After being compiled by TorchAir, the PyTorch model enters the GE path for execution.

#### Prerequisites

See [Enabling AutoFuse](autofuse_enable.md#enabling-autofuse-for-the-ge-path) to complete the environment setup, including installing PyTorch and the TorchNPU plugin, installing TorchAir, setting environment variables, and enabling automatic fusion. In this scenario, AutoFuse is enabled in the same way as in the TensorFlow scenario.

A PyTorch model must be compiled through the TorchAir backend before it can enter the GE path. AutoFuse performs operator fusion during the GE graph compilation phase:

```python
import torchair

# Create a compiler configuration object
config = torchair.CompilerConfig()

# Obtain the NPU backend object
# torchair.get_npu_backend is an API provided by TorchAir for initializing the NPU compilation backend
# compiler_config=config passes the previously created configuration to the backend to ensure the compilation behavior meets expectations
npu_backend = torchair.get_npu_backend(compiler_config=config)
# Compile the model
# model: the original PyTorch model to be compiled
# backend=npu_backend: specifies the NPU backend for compilation
# dynamic=True: Enable dynamic shape
model = torch.compile(
    model,
    backend=npu_backend,
    dynamic=True,
)
```

Here, `dynamic=True` is used to enable dynamic shape compilation. After the first compilation, the same compilation result can support inputs of different shapes without recompiling for each shape.

#### Example Code

This section uses the `Abs+ReLU+Exp` operator fusion as an example to demonstrate how to configure and run a fusion test case and how to verify the fusion result. The example uses four groups of inputs with the `float16` data type and different shapes, performs 100 inferences on the NPU, and enables NPU Profiling. After execution, a `profiling` directory is generated for viewing the fusion result and performance data.

```python
import torch
import torch_npu
import torchair
import torch.nn as nn

# Set the device to the first NPU card
DEVICE = "npu:0"
torch.npu.set_device(DEVICE)


class MyModel(nn.Module):
    def forward(self, x):
        return torch.exp(torch.relu(torch.abs(x)))


# Instantiate the model and move it to the NPU device
model = MyModel().to(DEVICE)

# ---Model compilation configuration---
# Create a TorchAir compiler configuration object
config = torchair.CompilerConfig()
# Obtain the NPU backend object for subsequent torch.compile calls
npu_backend = torchair.get_npu_backend(compiler_config=config)

# Compile the model using torch.compile
# backend=npu_backend: specify the NPU backend for graph optimization and code generation
# dynamic=True: enable dynamic shapes
model = torch.compile(
    model,
    backend=npu_backend,
    dynamic=True,
)

# Set the model to evaluation mode
model.eval()

# ---Prepare dynamic shape test inputs---
# Dynamic shape input: the same compilation result supports multiple shapes without recompilation
inputs = [
    torch.randn(shape, dtype=torch.float16, device=DEVICE)
    for shape in [(128, 192), (64, 256), (256, 64), (100, 100)]
]

# ---Profiling configuration---
# Configure the TorchNPU profiler to collect NPU runtime performance data
experimental_config = torch_npu.profiler._ExperimentalConfig(
    # Specify the export type as text format
    export_type=[torch_npu.profiler.ExportType.Text],
    # Set the profiling level to Level2 to collect more detailed performance metrics
    profiler_level=torch_npu.profiler.ProfilerLevel.Level2,
    # Whether to enable MSProf TX. Disabled here
    msprof_tx=False,
    # Collect AI Core utilization metrics
    aic_metrics=torch_npu.profiler.AiCMetrics.PipeUtilization,
    # Whether to collect L2 Cache information. Disabled here
    l2_cache=False,
    # Whether to collect operator attribute information. Disabled here.
    op_attr=False,
    # Whether to simplify data output. Disabled here to retain complete information.
    data_simplification=False,
    # Whether to record operator parameters. Disabled here.
    record_op_args=False,
    # GC detection threshold. Set to None to indicate no particular concern.
    gc_detect_threshold=None,
)

# ---Start performance profiling---
# Use the torch_npu.profiler.profile context manager to start profiling.
with torch_npu.profiler.profile(
    # Specify the activities to profile: record both CPU and NPU activities.
    activities=[
        torch_npu.profiler.ProfilerActivity.CPU,
        torch_npu.profiler.ProfilerActivity.NPU,
    ],
    on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("./profiling"),
    record_shapes=True,
    profile_memory=False,
    with_stack=False,
    with_modules=False,
    with_flops=False,
    experimental_config=experimental_config,
) as prof:
    # Iterate over all inputs with different shapes.
    for x in inputs:
        for _ in range(25):
            model(x)
```

#### Verify Fusion Result

After execution, a `profiling` directory is generated in the current directory. The `op_summary_*.csv` file exported by Profiling is usually located in the following path:

```text
profiling/
└── xxx_timestamp_ascend_pt/
    └── PROF_timestamp_xxx/
        └── mindstudio_profiler_output/
            └── op_summary_timestamp.csv
```

Open the `op_summary_*.csv` file corresponding to this run and check the operator list in it. If a fusion kernel whose name starts with `autofuse_` appears, it indicates that the related operators have been fused. In this example, the fusion kernel is named `autofuse_pointwise_0_Abs_Relu_Exp`. In dynamic shape scenarios, the same fusion kernel appears multiple times in the execution record with different input shapes. The specific kernel name may vary with the version, so you should determine it based on the operator type and the execution record.

#### Performance Comparison Before and After Fusion

To evaluate the performance gain of AutoFuse, collect Profiling data in the following two scenarios:

1. **AutoFuse enabled**: Configure the environment variable `AUTOFUSE_FLAGS="--enable_autofuse=true"`.
2. **AutoFuse disabled**: Configure the environment variable `AUTOFUSE_FLAGS="--enable_autofuse=false"` as the control group for the non-fusion scenario.

The two scenarios should use the same input, number of executions, and Profiling configuration. Compare the execution time within the same computation scope, and distinguish the first-time graph compilation overhead from the stable execution time after warm-up. For operators with a high proportion of input and output data transfer, you can further focus on `aiv_mte2_time` and `aiv_mte3_time` in the Profiling data.

For details about how to use the Profiling performance analysis tool, see [Performance Tuning Tool](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/devaids/Profiling/atlasprofiling_16_0001.html).

### ATC Scenario

This scenario supports only static shapes. To enable automatic fusion in offline inference scenarios, perform the following steps:

1. Set the environment variable to enable automatic fusion. For example:

    ```bash
    export AUTOFUSE_FLAGS="--enable_autofuse=true"
    ```

    - Use the ATC offline model conversion tool to convert the model and generate an om offline model. (In automatic fusion scenarios, converting a single operator to om is not supported, that is, the --singleop parameter is not supported.)
    - Use the [aclgrphBuildModel](../../../api/graph_engine_api/cpp/ge/aclgrphBuildModel.md) API to compile the model and generate an om offline model. (In automatic fusion scenarios, converting a single operator to om is not supported, that is, the INSERT\_OP\_FILE parameter is not supported.)

2. Model loading and inference

   Use the acl APIs to load the generated om model and perform inference.

    For details about how to use the ATC tool, see [Ascend Tensor Compiler (ATC)](../../atc_tools/README.md).

    For details about inference using the acl APIs, see *Model Inference* in [Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md)*.

## Inductor Path Enabling Automatic Fusion

This section uses the `add+ge` operator fusion as an example to demonstrate how to configure and run a fusion case and how to verify the fusion result. The example uses inputs with a shape of `[128, 50]` and a data type of `float32`, performs 100 inferences on the NPU, and has NPU Profiling built in. After execution, a `profiling` directory is generated, making it easy to view the fusion result and performance data.

### Prerequisites

See [Enabling AutoFuse](autofuse_enable.md#enabling-autofuse-for-the-inductor-path) to complete the environment setup, set environment variables, and enable the automatic fusion feature.

### Example Code

```python
import torch
import torch_npu
import torch.nn as nn

# Set the device to the first NPU card.
DEVICE = "npu:0"
torch.npu.set_device(DEVICE)


class MyModel(nn.Module):
    """
    Custom model: perform the comparison operation.
    Forward propagation logic: compute the boolean result of (x + y) >= z.
    """
    def forward(self, x, y, z):
        return torch.ge(torch.add(x, y), z)

# Instantiate the model and move it to the NPU device.
model = MyModel().to(DEVICE)
# Compile the model with torch.compile.
# options={"npu_backend": "ascendc"}: specify the Ascend C backend for compilation.
model = torch.compile(
    model,
    options={"npu_backend": "ascendc"},
)

# Create test input data.
# device=DEVICE ensures that the data is generated directly on the NPU.
x = torch.randn(128, 50, device=DEVICE)
y = torch.randn(128, 50, device=DEVICE)
z = torch.randn(128, 50, device=DEVICE)

# Set the model to evaluation mode.
model.eval()

# ---Profiling Configuration---
# Configure the torch_npu Profiler to collect performance data during NPU runtime.
experimental_config = torch_npu.profiler._ExperimentalConfig(
    # Specify the export type as text format.
    export_type=[torch_npu.profiler.ExportType.Text],
    # Set the profiling level to Level2 to collect more detailed performance metrics.
    profiler_level=torch_npu.profiler.ProfilerLevel.Level2,
    # Whether to enable MSProf TX. Disabled here.
    msprof_tx=False,
    # Collect AI Core utilization metrics.
    aic_metrics=torch_npu.profiler.AiCMetrics.PipeUtilization,
    # Whether to collect L2 Cache information. Disabled here.
    l2_cache=False,
    # Whether to collect operator attribute information. Disabled here.
    op_attr=False,
    # Whether to simplify data output. Disabled here to retain complete information.
    data_simplification=False,
    # Whether to record operator parameters. Disabled here.
    record_op_args=False,
    # GC detection threshold. Set to None to indicate no special attention.
    gc_detect_threshold=None,
)

# ---Start performance profiling---
# Use the torch_npu.profiler.profile context manager to start profiling.
with torch_npu.profiler.profile(
    # Specify the activities to profile: record both CPU and NPU activities.
    activities=[
        torch_npu.profiler.ProfilerActivity.CPU,
        torch_npu.profiler.ProfilerActivity.NPU,
    ],
    on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("./profiling"),
    record_shapes=True,
    profile_memory=False,
    with_stack=False,
    with_modules=False,
    with_flops=False,
    experimental_config=experimental_config,
) as prof:
    # Run the model 100 times repeatedly.
    for _ in range(100):
        model(x, y, z)

```

### Verify Fusion Result

After execution, a `profiling` directory is generated in the current directory. The `op_summary_*.csv` file exported by Profiling is usually located in the following path:

```text
profiling/
└── xxx_timestamp_ascend_pt/
    └── PROF_timestamp_xxx/
        └── mindstudio_profiler_output/
            └── op_summary_timestamp.csv
```

Open the `op_summary_*.csv` file corresponding to this run and check the operator list. If a fused kernel whose name starts with `autofused_` appears, the related operators have been fused. The specific kernel name may vary with the version, so determine the result based on the operator type and execution record.

### Performance Comparison Before and After Fusion

To evaluate the performance gain of AutoFuse, collect Profiling data in the following two scenarios:

1. **AutoFuse enabled**: Keep the `torch.compile(..., options={"npu_backend": "ascendc"})` configuration.
2. **AutoFuse disabled**: Comment out or remove the `torch.compile` configuration so that the model falls back to PyTorch Eager mode, which serves as the control for the unfused scenario.

Both scenarios should use the same input, number of executions, and Profiling configuration. Compare the execution time within the same computation scope, and distinguish the first-compilation overhead from the stable execution time after warm-up. For operators with a high proportion of input and output data movement, you can further examine `aiv_mte2_time` and `aiv_mte3_time` in the Profiling data.

For details about how to use the Profiling performance analysis tool, see [Performance Tuning Tool](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/devaids/Profiling/atlasprofiling_16_0001.html).
