# Quick Start

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:43:30.633Z pushedAt=2026-10-28T05:57:55.508Z nl=2 -->

## Running a Minimal Example in 10 Minutes

This sample uses the Add operator as an example to show how to connect an Ascend C custom operator to a GE graph through TorchAir. On the GE deliverable side, graph execution is implemented through EagerExecuteOp. On the Python side, both direct eager invocation and torch.compile graph-mode compilation invocation are supported. The brief process is as follows:

```plaintext
PyTorch invocation → FX graph → TorchAir converter → GE graph → GE callback Execute → launch Ascend C kernel
```

For the complete sample code, see [ascendc\_add\_custom](../../../../../examples/custom_op/ascendc_add_custom/README.md).

1. Obtain the sample

    ```bash
    git clone https://gitcode.com/cann/ge.git
    cd ge/examples/custom_op/ascendc_add_custom
    ```

    The complete directory structure is as follows:

    ```tree
    ascendc_add_custom
    ├── CMakeLists.txt             // Compile libcust_opapi.so and configure PyTorch/TorchAir/CANN dependencies
    ├── README.md
    ├── add_custom_kernel
    │   ├── add_custom.asc        // Ascend C kernel implementation and PyTorch registration code (TORCH_LIBRARY)
    │   ├── add_custom_kernel.h   // Kernel declaration
    │   └── custom_op.cpp         // GE deliverable, REG_OP + Execute + InferShape
    └── script
        └── add_custom_test.py    // Python test script (including converter registration, covering both eager and graph modes)
    ```

    - add\_custom\_kernel/add\_custom.asc: Implementation file of the Ascend C Add kernel, which also contains the PyTorch custom operator registration logic (TORCH\_LIBRARY\_FRAGMENT + TORCH\_LIBRARY\_IMPL).
    - add\_custom\_kernel/custom\_op.cpp: GE deliverable of the custom operator, which implements the Execute path, REG\_OP operator prototype registration, and InferShape inference, and completes GE graph integration.
    - script/add\_custom\_test.py: Python test script that covers eager execution and TorchAir graph-mode compilation and execution, and also includes converter registration.
    - CMakeLists.txt: Compiles libcust\_opapi.so and configures the PyTorch, TorchAir, and CANN dependencies.

2. Build

    Run the following command to build the project:

    ```bash
    mkdir -p build
    cd build
    cmake ..
    make -j"$(nproc)"
    ```

    After compilation, the custom operator deliverable libcust\_opapi.so is generated in the build path.

3. Deploy

    ```bash
    # Return to the sample root directory
    cd ..

    # Organize the output directory according to the OPP specification. (If the install rule is configured in the CMakeLists of the sample repository, you can use cmake --install build instead.)
    mkdir -p output/op_graph/lib/linux/x86_64
    cp build/libcust_opapi.so output/op_graph/lib/linux/x86_64/

    # Configure environment variables and verify
    export ASCEND_CUSTOM_OPP_PATH="$(pwd)/output:$ASCEND_CUSTOM_OPP_PATH"
    ls "$ASCEND_CUSTOM_OPP_PATH"/op_graph/lib/linux/x86_64/   # libcust_opapi.so should be displayed
    ```

4. Run Verification

    ```python
    python3 script/add_custom_test.py
    ```

    If "Ran 1 test" and "OK" appear in the terminal window, both the eager mode and the TorchAir graph mode have passed verification.

## Sample Analysis

1. **Stage 1: Operator Definition**

    The operator definition stage declares the metadata of the operator, such as its inputs, outputs, and attributes. In this sample, the GE-side REG\_OP definition and the PyTorch-side TORCH\_LIBRARY registration are implemented in two separate C++ files.

    1. **GE side REG\_OP (custom\_op.cpp):**

        ```c++
        // add_custom_kernel/custom_op.cpp
        // Register the GE operator prototype: declare the inputs, outputs, and supported data types of the AddCustom operator
        REG_OP(AddCustom)
            .INPUT(x, "T")    // Input x
            .INPUT(y, "T")    // Input y
            .OUTPUT(z, "T")   // Output z
            .DATATYPE(T, TensorType({DT_BOOL, DT_FLOAT, DT_INT32, DT_INT64, DT_FLOAT16, DT_BF16,
                                     DT_INT16, DT_INT8, DT_UINT8, DT_DOUBLE, DT_COMPLEX128,
                                     DT_COMPLEX64, DT_STRING, DT_COMPLEX32}))
            .OP_END_FACTORY_REG(AddCustom);
        ```

        The InferShape inference function is also implemented and registered in this file:

        ```c++
        // add_custom_kernel/custom_op.cpp
        // Shape inference function of the AddCustom operator, which implements PyTorch-style broadcast semantics. GE calls this function during graph compilation to infer the output shape from the input shapes
        graphStatus InferShapeForAdd(gert::InferShapeContext *context) {
          // Obtain the shapes of the two inputs
          auto input_shape_0 = *context->GetInputShape(0);
          auto input_shape_1 = *context->GetInputShape(1);
          // Obtain the writable reference to the output shape to set the inference result
          auto output_shape = context->GetOutputShape(0);
          // Case 1: The two inputs have different numbers of dimensions. Handle broadcast semantics (a 1D tensor is broadcast to multiple dimensions)
          if (input_shape_0.GetDimNum() != input_shape_1.GetDimNum()) {
            auto min_num = std::min(input_shape_0.GetDimNum(), input_shape_1.GetDimNum());
            auto max_num = std::max(input_shape_0.GetDimNum(), input_shape_1.GetDimNum());
            if (min_num != 1) {
              std::cerr << "Add param invalid" << std::endl;
            } else {
              if (input_shape_1.GetDimNum() > 1) {
                *output_shape = input_shape_1;
              } else {
                *output_shape = input_shape_0;
              }
              return GRAPH_SUCCESS;
            }
          }
          // Case 2: One input is a scalar (with zero dimensions). The output shape takes the shape of the other input
          if (input_shape_0.GetDimNum() == 0) {
            *output_shape = input_shape_1;
            return GRAPH_SUCCESS;
          }
          // Case 3: The two inputs have the same number of dimensions. Take the maximum value dimension by dimension (standard broadcast rule)
          if (input_shape_1.GetDimNum() == 0) {
            *output_shape = input_shape_0;
            return GRAPH_SUCCESS;
          }
          output_shape->SetDimNum(input_shape_0.GetDimNum());
          for (size_t i = 0; i < input_shape_0.GetDimNum(); ++i) {
            output_shape->SetDim(i, std::max(input_shape_0.GetDim(i), input_shape_1.GetDim(i)));
          }
          return GRAPH_SUCCESS;
        }

        // Register the InferShape inference function with the AddCustom operator. When GE encounters an AddCustom node at compile time, it calls this function to infer the output shape
        IMPL_OP(AddCustom).InferShape(InferShapeForAdd);
        ```

    2. **PyTorch-side TORCH\_LIBRARY registration (add\_custom.asc):**

        In the .asc file, define the operator schema through TORCH\_LIBRARY\_FRAGMENT and bind the implementation of each DispatchKey through TORCH\_LIBRARY\_IMPL:

        ```c++
        // add_custom_kernel/add_custom.asc
        // PyTorch-side operator implementation: execute the Ascend C kernel on the NPU
        namespace ascendc_ops {
        // The ascendc_add function: called by PyTorch in eager mode
        // Prepare parameters and launch the Ascend C kernel function
        at::Tensor ascendc_add(const at::Tensor& x, const at::Tensor& y) {
          // Obtain the current NPU execution stream, on which the kernel runs asynchronously
          auto aclStream = c10_npu::getCurrentNPUStream().stream(false);
          // Allocate the output tensor, whose shape is the same as that of input x
          at::Tensor z = at::empty_like(x);
          // Set the number of blocks for kernel function launch
          uint32_t numBlocks = 8;
          // Calculate the total number of input elements (product of all dimensions)
          uint32_t totalLength = 1;
          for (uint32_t size : x.sizes()) {
            totalLength *= size;
          }
          // Call the wrapper function to launch the kernel function
          launch_add_custom(
            static_cast<uint8_t*>(x.data_ptr()),  // Device address of input x
            static_cast<uint8_t*>(y.data_ptr()),  // Device address of input y
            static_cast<uint8_t*>(z.data_ptr()),  // Device address of output z
            totalLength,                          // Total number of elements
            numBlocks,                            // Number of kernel blocks
            aclStream                             // Execution stream
          );
          return z;
        }
        } // namespace ascendc_ops

        // ===== PyTorch operator registration =====
        // Define the operator schema: declare the function signature for PyTorch to recognize
        // ascendc_ops is the namespace, and ascendc_add is the function name
        TORCH_LIBRARY_FRAGMENT(ascendc_ops, m) {
          m.def("ascendc_add(Tensor x, Tensor y) -> Tensor");
        }

        // Register the PrivateUse1 implementation: the eager execution path on NPU devices
        // Triggered when the user directly calls torch.ops.ascendc_ops.ascendc_add(x, y) on the NPU
        TORCH_LIBRARY_IMPL(ascendc_ops, PrivateUse1, m) {
          m.impl("ascendc_add", TORCH_FN(ascendc_ops::ascendc_add));
        }

        // Register the XLA implementation: the graph compilation path
        // When the operator enters graph mode through torch.compile, the XLA path triggers GE graph compilation
        TORCH_LIBRARY_IMPL(ascendc_ops, XLA, m) {
          m.impl("ascendc_add", TORCH_FN(ascendc_ops::ascendc_add));
        }

        // Register the Meta implementation: compile-time shape inference
        // The Meta path does not perform actual computation and only returns an empty Tensor with the same shape as the input
        // TorchAir calls this implementation for shape inference during graph compilation
        TORCH_LIBRARY_IMPL(ascendc_ops, Meta, m) {
          m.impl("ascendc_add", [](const torch::Tensor& x, const torch::Tensor& y) {
            return at::empty_like(x);
          });
        }
        ```

        **Note:** In this sample, the PyTorch registration logic is placed in the .asc file (C++ source code) and compiled in the same library as the kernel implementation. This is a common practice for integrating Ascend C operators into graphs, where the kernel and registration logic are completed in a single compilation unit.

2. **Stage 2: Frontend Integration**

    Frontend integration maps PyTorch FX nodes to GE custom operators through the TorchAir converter. In this sample, the converter is registered in the test script:

    ```python
    # script/add_custom_test.py
    import torchair
    from torchair import register_fx_node_ge_converter
    from torchair.ge import Tensor, TensorSpec

    # Register the GE converter: map the custom operator nodes in the PyTorch FX graph to operator nodes in the GE graph
    # The decorator specifies the PyTorch operator reference to be converted (torch.ops.ascendc_ops.ascendc_add.default)
    # TorchAir calls this converter function when it encounters this operator during graph compilation
    @torchair.register_fx_node_ge_converter(torch.ops.ascendc_ops.ascendc_add.default)
    def convert_ascendc_ascendc_add(
        x: Tensor, y: Tensor, z: Tensor = None, meta_outputs: TensorSpec = None
    ):
        # Construct and return the GE custom operator node
        return torchair.ge.custom_op(
            "AddCustom",            # GE operator type name, which must be consistent with REG_OP and REG_AUTO_MAPPING_OP
            inputs={
                "x": x,             # Map the PyTorch input x to the input port x of the GE operator
                "y": y,             # Map the PyTorch input y to the input port y of the GE operator
            },
            outputs=["z"],          # Declare the output port name of the GE operator, corresponding to .OUTPUT(z, ...) in REG_OP
        )
    ```

    > [!NOTE]
    > The "AddCustom" in the converter must be consistent with the op type in REG\_OP and REG\_AUTO\_MAPPING\_OP.

3. **Stages 4-5: Implementing the GE deliverables**

    The GE deliverables implement EagerExecuteOp::Execute in custom\_op.cpp to launch the kernel:

    ```c++
    // add_custom_kernel/custom_op.cpp
    // Implementation class of the GE deliverables for the AddCustom operator, which inherits EagerExecuteOp and implements the Execute method to launch the kernel
    class AddCustom : public EagerExecuteOp {
     public:
      // Execute callback: GE calls this method during the graph execution phase to launch the Ascend C kernel function
      graphStatus Execute(gert::EagerOpExecutionContext *ctx) {
        // Obtain the input tensor of the operator (including metadata such as device address, shape, dtype, and format)
        const gert::Tensor *input_x = ctx->GetInputTensor(0);
        const gert::Tensor *input_y = ctx->GetInputTensor(1);
        if (input_x == nullptr || input_y == nullptr) {
          std::cerr << "Input tensor is null!" << std::endl;
          return GRAPH_FAILED;
        }
        // 2. Extract metadata from the input tensor to allocate the output tensor
        const gert::StorageShape &output_shape = input_x->GetShape();
        uint32_t tensor_size = input_x->GetSize();
        DataType data_type = input_x->GetDataType();
        const gert::StorageFormat &format = input_x->GetFormat();

        // 3. Allocate device memory for the output tensor (managed by the GE context, no manual release required)
        gert::Tensor *output_z = ctx->MallocOutputTensor(0, output_shape, format, data_type);
        if (output_z == nullptr) {
          std::cerr << "Failed to malloc output tensor memory!" << std::endl;
          return GRAPH_FAILED;
        }
        // 4. Calculate the kernel launch parameters: grid dimensions and block count
        int64_t n_elements = input_x->GetShapeSize();
        const int32_t BLOCK_SIZE_VALUE = 1024;
        int32_t grid_x = std::ceil(static_cast<double>(n_elements) / (BLOCK_SIZE_VALUE));
        int32_t grid_y = 1;
        int32_t grid_z = 1;
        uint32_t num_block = grid_x * grid_y * grid_z;

        // 5. Obtain the device addresses of the input and output
        void *z_addr = output_z->GetAddr();
        void *x_addr = const_cast<void *>(input_x->GetAddr());
        void *y_addr = const_cast<void *>(input_y->GetAddr());
        // 6. Obtain the execution stream and launch the kernel (asynchronous execution, returns immediately)
        aclrtStream stream = ctx->GetStream();
        // Call the wrapper function to launch the Ascend C kernel add_custom
        launch_add_custom(static_cast<uint8_t *>(x_addr), static_cast<uint8_t *>(y_addr),
                          static_cast<uint8_t *>(z_addr), tensor_size, num_block, stream);
        return GRAPH_SUCCESS;
      }
    };

    // Register the AddCustom implementation class with GE
    // When GE encounters an "AddCustom" type node in the graph, it instantiates this class and calls back the Execute method
    REG_AUTO_MAPPING_OP(AddCustom);
    ```

    Here, launch\_add\_custom is declared in add\_custom\_kernel.h and implemented in add\_custom.asc, and is responsible for calling the Ascend C kernel add\_custom.

## Debugging and Troubleshooting

During the preceding development process, if GE cannot find the custom calculation operator, or ASCEND\_CUSTOM\_OPP\_PATH is not configured, or the .so file is not in the op\_graph/lib/<os\>/<arch\>/ path, see [Debugging and Common Issues](debug_and_common_issues.md).
