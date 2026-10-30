# Integrating a Triton Operator into a Graph

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:01:11.936Z pushedAt=2026-10-28T03:30:34.444Z nl=2 -->

## Overview

Triton is a Python-based programming language that has been favored by developers in recent years. Developers only need to focus on the tile/block slicing mode and the computation logic based on tiles/blocks. During the compilation of Triton code, the Triton compiler automatically completes memory allocation, data transfer, data computation, and pipeline parallelism based on the characteristics of underlying hardware. This greatly reduces the operator development difficulty and significantly improves the development efficiency.

Triton-Ascend is a Triton compilation framework built for the Ascend platform, aiming to enable Triton code to run efficiently on Ascend hardware. Triton-Ascend is still being improved. We will continuously improve the completeness of Triton Python APIs, support more data types, make memory access more flexible, and continuously optimize the automatic optimization capability of the compiler to improve the overall functionality and performance generalization of Triton-Ascend.

For users, integrating Triton operators into the GE graph allows them to reuse the Triton custom operators developed during GPU training and incorporate them into the graph execution process at minimal cost, thereby enjoying the ultimate performance benefits brought by graph sinking.

The overall graph process is shown in the following figure.

![Illustration](../figures/triton_irgraph.png)

This feature is mainly divided into two phases: the development phase and the runtime phase:

- Development phase:
    1. The user needs to complete the environment configuration. For details, see [Environment Setup](#environment-setup).
    2. Compile the Triton operator kernel into the corresponding npubin file. For details, see [Generating the npubin File](#generating-the-npubin-file).
    3. [Developing the TensorFlow Graph Deliverable](#developing-the-tensorflow-graph-deliverable). Currently, only the TensorFlow framework is supported.
    4. Create the Triton custom operator GE graph deliverable. This step includes creating the operator graph integration project, developing the operator graph deliverable, and compiling and deploying it. For details, see [Developing the GE Graph Deliverable](#developing-the-ge-graph-deliverable).

- Runtime phase: You can call the TensorFlow custom operator in the normal way. With the help of GE, the model is parsed into a graph. After graph compilation and graph execution, the graph sinking mechanism can be fully leveraged to significantly improve performance. For details, see [Result Verification](#result-verification).

## Development Process

This section describes the detailed development process for integrating Triton custom operators into the GE graph.

### Environment Setup

- **CANN Software Package Installation**: See [Environment Setup](../overview/environment_setup.md) to install the CANN software package and set environment variables.
- **Triton-Ascend Installation**: See the [Triton-Ascend Installation Guide](https://gitcode.com/Ascend/triton-ascend/blob/master/docs/sources/getting-started/installation.md) for installation.

For a detailed sample, click [Triton Custom Operator Integration Sample](../../../../../examples/custom_op/triton_add_custom/README.md).

### Generating the npubin File

You need to call the compile API provided by Triton-Ascend to compile the operator kernel and generate the corresponding npubin file. Taking the custom operator AddCustom as an example (with the file name AddCustom.py as an example), the sample code is as follows:

```python
# Compute Kernel
import torch
import torch_npu   # torch_npu plugin

import triton
import triton.language as tl  # Triton language module

# Define a Triton kernel function to implement vector addition of x+y
@triton.jit
def add_kernel(x_ptr,  # Pointer to the first input vector
               y_ptr,  # Pointer to the second input vector
               output_ptr,  # Pointer to the output vector
               n_elements,  # Length of the vector
               BLOCK_SIZE: tl.constexpr,  # Number of elements processed by each program
               ):
    # Obtain the ID of the current program.
    pid = tl.program_id(axis=0)  # We use a 1D launch grid so axis is 0.
    # Calculate the starting position processed by the current program.
    block_start = pid * BLOCK_SIZE
    # Generate the offset list of elements processed by the current program.
    offsets = block_start + tl.arange(0, BLOCK_SIZE)
    # Create a mask to prevent out-of-bounds access.
    mask = offsets < n_elements
    # Load x and y from DRAM, using the mask to avoid out-of-bounds access.
    x = tl.load(x_ptr + offsets, mask=mask)
    y = tl.load(y_ptr + offsets, mask=mask)
    # Calculate x+y.
    output = x + y
    # Write the result back to DRAM.
    tl.store(output_ptr + offsets, output, mask=mask)
# Define the block size.
BLOCK_SIZE_VALUE = 1024
# Create the Triton AST source code object.
add_kernel_ast = triton.compiler.ASTSource(
    fn=add_kernel,
    signature={
        'x_ptr': '*fp32',
        'y_ptr': '*fp32',
        'output_ptr': '*fp32',
        'n_elements': 'i32',
    },
    constants={
        'BLOCK_SIZE': BLOCK_SIZE_VALUE  # Define the block size as a constant.
    }
)
# Compile the kernel
compiled = triton.compile(add_kernel_ast)
```

Run the following command in the directory of the preceding script:

```bash
python3  AddCustom.py
```

After successful execution, the npubin, json, ttir, and ttadapter files are generated in the \~/.triton/cache directory, as shown in the following figure.

**Figure 1** Files in the cache directory
![Figure 1 Example](../figures/cache_directory_files.png "Files in the cache directory")

### Developing the TensorFlow Graph Deliverable

TensorFlow natively supports custom operators. Therefore, you need to provide a graph deliverable for a TensorFlow custom operator so that TensorFlow can recognize this custom operator. Taking TensorFlow 1.15 as an example, create the prototype registration file _custom\_add\_custom.cc_ with the following content:

```c++
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/shape_inference.h"
#include "tensorflow/core/framework/common_shape_fns.h"

using namespace tensorflow;
// Register the operator prototype through the REGISTER_OP API provided by TensorFlow
REGISTER_OP("AddCustom")                                    // TensorFlow registered operator name
    .Input("x: T")                                          // Operator prototype, input parameter x of type T
    .Input("y: T")                                          // Operator prototype, input parameter y of type T
    .Output("z: T")                                         // Operator prototype, output parameter z of type T
    .Attr("T: {float, double, int32}")                      // Supported range of type T
    .SetShapeFn(shape_inference::BroadcastBinaryOpShapeFn); // Operator shape inference. BroadcastBinaryOpShapeFn is a built-in function provided by TensorFlow. The output shape is inferred by propagating the input shape, that is, the input and output shapes remain consistent


// CPU implementation of the TensorFlow custom operator
class AddCustomOp : public OpKernel {
public:
    explicit AddCustomOp(OpKernelConstruction* context) : OpKernel(context) {}
    // The current operator does not support CPU devices. Implement this function to throw an exception indicating that the operator does not support CPU devices.
    void Compute(OpKernelContext* context) override {
        OP_REQUIRES(context, false, errors::Unimplemented("AddCustomOp is not supported on CPU"));
    }
};

// Register the CPU implementation of the TensorFlow custom operator
REGISTER_KERNEL_BUILDER(Name("AddCustom").Device(DEVICE_CPU), AddCustomOp);
```

Use the following command to compile the preceding code. After successful compilation, an artifact named libcustom\_ops.so is generated in the outputs directory under the current path. In subsequent operator invocation scripts, you can load the .so file as a Python module through the load\_op\_library API to invoke the custom operator.

```bash
#! /bin/bash
SCRIPT_DIR=$(dirname "(realpath "$0")")
cd $SCRIPT_DIR || exit

rm -rf outputs
mkdir outputs

TF_CFLAGS=( $(python3 -c 'import tensorflow as tf; print(" ".join(tf.sysconfig.get_compile_flags()))') )    // Obtain the TensorFlow compilation options
TF_LFLAGS=( $(python3 -c 'import tensorflow as tf; print(" ".join(tf.sysconfig.get_link_flags()))') )       // Obtain the TensorFlow link options

SOURCE_FILES=$(find . -name '*.cc')                                                                         // Include the cc files for TensorFlow operator registration and CPU kernel implementation

g++ -std=c++14 -shared $SOURCE_FILES -o outputs/libcustom_ops.so -fPIC ${TF_CFLAGS[@]} ${TF_LFLAGS[@]} -O2  // Compilation command. The artifact is libcustom_ops.so, which TensorFlow can load as a Python module through load_op_library to invoke the custom operator.
```

### Developing the GE Graph Deliverable

1. Download the project for integrating Triton custom operators into the graph to any directory. Its internal structure is as follows:

    ```text
    custom_op/
    ├── src/
    |   └── custom_op.cpp               // Source file of the custom operator
    ├── CMakeLists.txt                  // CMake file
    ├── gen_npu_supported_ops_json.sh   // Script for generating files
    ├── README.md                       // Readme
    └── run.sh                          // Run script
    ```

2. Go to the src directory of the custom project and write the custom\_op.cpp file. An example is as follows:

    In the example, AddCustom is the implementation class of the user-defined Triton custom operator. It inherits from [EagerExecuteOp](../../../api/graph_engine_api/cpp/ge/EagerExecuteOp/EagerExecuteOp.md/) and mainly implements the [Execute](../../../api/graph_engine_api/cpp/ge/EagerExecuteOp/Execute.md) function.

    The main process is as follows:

    1. Load the operator binary from the npubin file.
    2. Obtain the kernel handle.
    3. Construct the kernel parameter structure.
    4. Call the acl Launch Kernel API to start the computation task of the corresponding operator.

    ```c++
    #include "graph/custom_op.h"
    #include "acl/acl_rt.h"
    #include "exe_graph/runtime/eager_op_execution_context.h"
    using namespace ge;
    class AddCustom : public EagerExecuteOp {
     public:
      graphStatus Execute(gert::EagerOpExecutionContext *ctx) {
        // Obtain the kernel handle from the npubin file
        const char *bin_path = "./add_kernel.npubin";        // Path of the binary file of the custom operator
        aclrtBinHandle bin_handle = nullptr;                 // Binary handle
        aclrtFuncHandle func_handle = nullptr;               // Kernel function handle
        // Configure the binary loading options
        aclrtBinaryLoadOption binary_load_option;
        aclrtBinaryLoadOptions binary_load_options;
        binary_load_option.type = ACL_RT_BINARY_LOAD_OPT_LAZY_LOAD;  // Specify whether to load the operator to the Device side after parsing the operator binary and registering the operator
        binary_load_option.value.isLazyLoad = 0U;

        binary_load_options.numOpt = 1;
        binary_load_options.options = &binary_load_option;

        // Load the operator binary file
        aclError ret = ACL_ERROR_NONE;
        ret = aclrtBinaryLoadFromFile(bin_path, &binary_load_options, &bin_handle);
        if (ret != ACL_ERROR_NONE) {
          std::cerr << __FILE__ << ":" << __LINE__ << " aclError: " << ret << std::endl;
        }
        // Obtain the handle of the kernel function named "add_kernel" from the binary
        ret = aclrtBinaryGetFunction(bin_handle, "add_kernel", &func_handle);
        if (ret != ACL_ERROR_NONE) {
          std::cerr << __FILE__ << ":" << __LINE__ << " aclError: " << ret << std::endl;
        }

        // Obtain the input tensors of the operator
        const gert::Tensor *input_x = ctx->GetInputTensor(0);
        const gert::Tensor *input_y = ctx->GetInputTensor(1);
        // Obtain the data address
        void *x_addr = input_x->GetAddr();
        void *y_addr = input_y->GetAddr();

        // Apply for device memory for the output tensor
        StorageShape &output_shape = input_x->GetShape();
        size_t tensor_size = input_x->GetSize();
        DataType data_type = input_x->GetDataType();
        const StorageFormat &format = input_x->GetFormat();
        gert::Tensor *output_z =
            ctx->MallocOutputTensor(0, output_shape, format, data_type, tensor_size, gert::TensorPlacement::kOnDeviceHbm);
        void *z_addr = output_z->GetAddr();

        // Obtain the number of elements to be processed and the grid
        int64_t n_elements = input_x->GetShapeSize();
        int32_t grid_x = 16;
        int32_t grid_y = 1;
        int32_t grid_z = 1;
        int32_t block_num = grid_x * grid_y * grid_z;

        // Assemble args and construct the kernel parameter structure
        // The first three and last three parameters of args are fixed, and the middle ones are user-defined. They must strictly match the signature and types of the kernel function.
        // According to the kernel implementation in this sample, the two inputs of AddCustom must have the same shape. Broadcast is not supported. For example, Shape1 = [2,3,4] and Shape2 = [4] is not supported.
        struct __attribute__((packed)) {
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
            nullptr, nullptr, input_x->GetAddr(), input_y->GetAddr(), z_addr, static_cast<int32_t>(n_elements),
        };
        // Obtain the stream
        void *stream = ctx->GetStream();
        // Launch the kernel function (asynchronous execution)
        // Function prototype:
        // aclError aclrtLaunchKernelWithHostArgs(aclrtFuncHandle func_handle,
        //                                      uint32_t block_num,
        //                                      aclrtStream stream,
        //                                      aclrtLaunchKernelCfg *cfg,
        //                                      void *hostArgs,
        //                                      size_t size,
        //                                      aclrtPlaceHolderInfo *placeHolderArray,
        //                                      size_t placeHolderNum);
        // func_handle[in]: Kernel function handle
        // block_num  [in]: Specifies the number of cores on which the kernel function runs
        // stream     [in]: Task stream
        // cfg        [in]: Configuration information for task delivery. Set to nullptr if not needed
        // hostArgs   [in]: Address of the kernel function parameters
        // size       [in]: Size of the parameters in bytes
        // placeHolderArray[in]: Placeholder array
        // placeHolderNum[in]: Number of elements in the placeholder array
        ret = aclrtLaunchKernelWithHostArgs(func_handle, static_cast<uint32_t>(block_num), stream, nullptr,
                                            static_cast<void *>(&args), sizeof(args), nullptr, 0);
        if (ret != ACL_ERROR_NONE) {
          std::cerr << __FILE__ << ":" << __LINE__ << " aclError: " << ret << std::endl;
        }
        return GRAPH_SUCCESS;
      }
    };
    // Register the custom operator so that it can be recognized and invoked during graph compilation. The registered name "AddCustom" corresponds to the operator name in the graph and must be consistent with the frontend configuration.
    REG_AUTO_MAPPING_OP(AddCustom);
    ```

    <!-- npu="A3" id1 -->
    **Note:**

    If the device is an Atlas A3 product, some structure fragments in the preceding code must be replaced with the following form. No modification is required for other product models:

    ```c++
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
    nullptr, nullptr, nullptr, input_x->GetAddr(), input_y->GetAddr(), z_addr, static_cast<int32_t>(n_elements),
    };
    ```
    <!-- end id1 -->

    For details about the aclrtBinaryLoadFromFile, aclrtBinaryGetFunction, and aclrtLaunchKernelWithHostArgs APIs, see "Kernel Loading and Execution" in *[Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md)*.

    For details about the GetAddr, GetShape, GetSize, GetDataType, GetFormat, and GetShapeSize APIs, see *[Basic Data Structures and APIs](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md)*.

    For details about other APIs, see [EagerExecuteOp](../../../api/graph_engine_api/cpp/ge/EagerExecuteOp/EagerExecuteOp.md/), [Execute](../../../api/graph_engine_api/cpp/ge/EagerExecuteOp/Execute.md), [GetInputTensor](../../../api/graph_engine_api/cpp/gert/EagerOpExecutionContext/GetInputTensor.md), [MallocOutputTensor](../../../api/graph_engine_api/cpp/gert/EagerOpExecutionContext/MallocOutputTensor.md), [GetStream](../../../api/graph_engine_api/cpp/gert/EagerOpExecutionContext/GetStream.md), [REG\_AUTO\_MAPPING\_OP](../../../api/graph_engine_api/cpp/ge/REG_AUTO_MAPPING_OP.md).

3. After the development is complete, run the run.sh script under custom\_op to compile:

    ```bash
    bash run.sh
    ```

    If the following information is displayed, the build is successful:

    ![Illustration](../figures/triton_irgraph_1.png)

    Set the environment variables as prompted above:

    ```bash
    export ASCEND_CUSTOM_OPP_PATH=/home/your_path/TritonOp/custom_op/build_out:$ASCEND_CUSTOM_OPP_PATH
    ```

### Result Verification

After all the preceding steps are completed, you can use the following script to verify the integration of the custom operator into the GE graph. The following is a sample call code based on TensorFlow 1.15, assuming the file name is run\_add\_custom\_tf\_1.15.py:

```python
#!/usr/bin/python3
# coding=utf-8

import os
import tensorflow as tf
import numpy as np
from npu_bridge.npu_init import *  # Import the NPU initialization module
tf.enable_resource_variables()     # Enable resource variables

# Relative tolerance parameter of the np.allclose comparison function
atol = 0.001
# Absolute tolerance parameter of the np.allclose comparison function
rtol = 0.001

def main(unused_argv):
    custom_op_lib = tf.load_op_library(os.path.join("./outputs/libcustom_ops.so")) # Load the custom operator library. Modify the path based on the compilation output directory in [Developing the TensorFlow Graph Deliverable](#developing-the-tensorflow-graph-deliverable)
    # Define the input data
    shape_params = (8, 2048)
    dtype_params = np.float32

    # Generate two random input tensors with values between -2 and 2
    x_data = np.random.uniform(-2, 2, size=shape_params).astype(dtype_params)
    y_data = np.random.uniform(-2, 2, size=shape_params).astype(dtype_params)

    # Define the input placeholders (TensorFlow 1.x style)
    x = tf.compat.v1.placeholder(dtype_params, shape=shape_params)
    y = tf.compat.v1.placeholder(dtype_params, shape=shape_params)

    # Define two addition operations
    tf_z = tf.math.add(x, y)                 # Call the TensorFlow native operator (TensorFlow built-in addition operation)
    ac_z = custom_op_lib.add_custom(x, y)    # Call the AddCustom custom operator written in Triton;

    # Configure the Session to support NPU acceleration
    config = tf.ConfigProto()
    custom_op = config.graph_options.rewrite_options.custom_optimizers.add()
    # Use the NPU optimizer to enable dynamic compilation mode
    custom_op.name = "NpuOptimizer"
    # Disable remapping and memory optimization to avoid interfering with custom operator execution
    config.graph_options.rewrite_options.remapping = RewriterConfig.OFF
    config.graph_options.rewrite_options.memory_optimization = RewriterConfig.OFF
    custom_op.parameter_map["compile_dynamic_mode"].b = True

    # First Session: run the TensorFlow built-in addition operation to obtain the reference result (golden)
    with tf.Session(config=config) as sess:
        sess.run(tf.global_variables_initializer())   # Initialize variables
        tf_golden = sess.run(tf_z, feed_dict={x: x_data, y: y_data})  # Execute the TensorFlow addition

    # Second Session: run the Triton custom addition operation to obtain the result
    with tf.Session(config=config) as sess:
        sess.run(tf.global_variables_initializer())    # Initialize variables.
        ac_golden = sess.run(ac_z, feed_dict={x: x_data, y: y_data})  # Execute the custom addition.

   # Convert the result to the specified data type to ensure comparison consistency.
    np.array(tf_golden).astype(dtype_params)
    np.array(ac_golden).astype(dtype_params)

    # Use np.allclose to compare whether the TensorFlow and Triton outputs are consistent.
    cmp_result = np.allclose(tf_golden, ac_golden, atol=atol, rtol=rtol)
    if cmp_result:
        print("The result of tf and ac is the same.")
    else:
        print("The result of tf and ac is different.")


if __name__ == '__main__':
    tf.app.run()
```

Run the preceding file by executing the following command:

```bash
python3 run_add_custom_tf_1.15.py > triton.log
```

After the execution is complete, enter the following command:

```bash
cat triton.log | grep "The result of tf and ac is the same"
```

If the following information can be found in the result, the use case is executed successfully:

![Illustration](../figures/triton_irgraph_2.png)
