# PyTorch Graph Integration

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:41:51.939Z pushedAt=2026-10-28T05:56:27.327Z nl=2 -->

This chapter uses **PyTorch+TorchAir** as the frontend framework to describe in detail the complete process of integrating custom operators into the GE graph.

## Prerequisites

See [Environment Preparation](env_preparation.md) to set up the environment.

## Graph Integration Stage

Integrating a custom operator into a graph consists of five progressive stages. Developers can complete graph integration by finishing the deliverables of each stage in order. A brief diagram of the deliverables on both sides is as follows:

![](../figures/pytorch_into_graph.png)

The following describes each stage and its deliverables in detail.

1. **Stage 1: Operator Definition**

    In the operator definition stage, metadata such as the inputs, outputs, and attributes of the operator is declared for use in subsequent stages. In the PyTorch scenario, the operator must be **defined in parallel on both sides**:

    | Frontend Framework | Definition Method | Deliverable | Description |
    | --- | --- | --- | --- |
    | GE side | REG_OP header file | op_graph/include/*.h | Manually written proto definition |
    | PyTorch side | TORCH_LIBRARY | python/*.py | Defines the schema (function signature) |

    - **GE side REG\_OP example:**

        ```c++
        // Register the GE operator prototype: define the inputs, outputs, and attributes of the operator.
        // op_graph/include/add_custom.h
        REG_OP(AddCustom)
            .INPUT(x, TensorType({DT_FLOAT, DT_FLOAT16}))    // Declare the input x.
            .INPUT(y, TensorType({DT_FLOAT, DT_FLOAT16}))    // Declare input y
            .OUTPUT(z, TensorType({DT_FLOAT, DT_FLOAT16}))   // Declare output z
            .OP_END_FACTORY_REG(AddCustom);
        ```

    - **PyTorch-side TORCH\_LIBRARY example:**

        ```python
        # python/add_custom.py
        import torch

        # Define the operator schema, which must be consistent with the inputs and outputs of REG_OP
        torch.library.define("my_ops", "add_custom(Tensor x, Tensor y) -> Tensor")
        ```

        **Consistency requirement**: The number and types of inputs/outputs in the PyTorch schema must be strictly consistent with the REG\_OP definition; otherwise, an error is reported during graph conversion.

2. **Stage 2: Frontend integration**

    The frontend integration stage maps PyTorch FX nodes to GE operator types. In the PyTorch scenario, this is implemented through the **TorchAir converter** mechanism, which requires the cooperation of deliverables on both sides:

    **The Python side requires:**

    1. TORCH\_LIBRARY defines the operator schema.
    2. TORCH\_LIBRARY\_IMPL registers keys such as PrivateUse1 (NPU execution), Meta (shape derivation), and XLA (graph compilation).
    3. @torchair.register\_fx\_node\_ge\_converter maps the PyTorch FX node to the GE operator type.

    **Python-side integration code:**

    ```python
    # python/add_custom_converter.py
    import torch
    import torchair

    # Define the operator schema
    torch.library.define("my_ops", "add_custom(Tensor x, Tensor y) -> Tensor")

    # Register the implementation (PrivateUse1 = NPU execution)
    @torch.library.impl("my_ops.add_custom", "PrivateUse1")
    def add_custom_impl(x, y):
        return torch.empty_like(x)  # placeholder, actually executed by GE

    # Register the GE converter: map the FX node to the GE operator
    @torchair.register_fx_node_ge_converter(torch.ops.my_ops.add_custom.default)
    def convert_add_custom(x, y, meta_outputs=None):
        return torchair.ge.custom_op(
            "AddCustom",           # GE op type (must be consistent with REG_AUTO_MAPPING_OP)
            inputs={"x": x, "y": y},
            outputs=["z"]
        )
    ```

    > [!NOTE] Note
    >**Converter signature convention**: The positional parameters correspond to the operator inputs (x, y) in order, and meta\_outputs is the compile-time output specification passed in by TorchAir. **Do not** write the output as a positional parameter in the signature.

    **Deliverable organization example:**

    ```tree
    custom_opp/
    ├── op_graph/
    │   ├── include/add_custom.h    # REG_OP
    │   └── lib/linux/x86_64/libcust_opapi.so  # GE deliverable
    └── python/
        ├── add_custom.py           # TORCH_LIBRARY definition
        └── add_custom_converter.py # converter registration
    ```

    For the unified deliverable specifications, see [Deliverables and OPP Specifications](deliverables_and_opp.md).

3. **Stage 3: Graph Construction and Graph Integration**

    In the graph construction and insertion stage, the PyTorch FX Graph is automatically converted into a GE compute graph.

    | Frontend Framework | Graph Integration Path | Conversion Mechanism |
    | --- | --- | --- |
    | PyTorch | FX Graph → GE Graph | TorchAir automatic conversion |

    This stage is automatically completed by TorchAir, and **no additional deliverables are required from developers**. After graph insertion is complete, the GE graph already contains the custom operator nodes.

4. **Stage 4: Graph Compilation**

    The callback interfaces for graph compilation are triggered during GE compilation and are used for shape derivation, kernel compilation, and serialization. **Callback interfaces and trigger timing:**

    | Callback Interface | Trigger Time | Applicable Scenarios |
    | --- | --- | --- |
    | InferShape/InferDataType | Shape/DataType derivation at compile time | All scenarios (optional) |
    | Compile | RTC kernel compilation at compile time | Online compilation/offline OM scenario |
    | Serialize | Writing to OM during ATC compilation | Offline OM scenario |

    For the signature, context APIs, and typical process, see [Callback Interface](callback_interface.md).

5. **Stage 5: Graph Execution**

    This stage is the core of the custom operator. During execution, GE calls back the Execute interface.

    **Callback interfaces and trigger timing:**

    | Callback Interface | Trigger Time | Applicable Scenarios |
    | --- | --- | --- |
    | Execute | Executes the kernel at runtime | All scenarios (must be implemented) |
    | Deserialize | Restores the binary when the OM is loaded | Offline OM scenario |
    | UpdateHostArgs | Refreshes when the I/O address changes | Static graph sinking scenario |

    For a detailed example, see [Sample Analysis](quick_start.md#sample-analysis).

**Summary: PyTorch graph insertion stages and deliverables comparison table:**

| Stage | Involved Deliverable | Description |
| --- | --- | --- |
| Stage 1: Operator Definition | op_graph/include/.h + python/.py | Proto header files, etc. |
| Stage 2: Frontend Integration | python/*_converter.py | Parses the plugin .so, etc. |
| Stage 3: Graph Construction and Insertion | None | No additional deliverable (automatically converted by TorchAir) |
| Stage 4: Graph Compilation | op_graph/lib/<os\>/<arch\>/*.so | Callback implementation in the GE deliverable |
| Stage 5: Graph Execution | op_graph/lib/<os\>/<arch\>/*.so | Callback implementation in the GE deliverable |

## Project Compilation and Deployment

The key elements of the CMake build are as follows:

```cmake
# Header file paths (must be included)
target_include_directories(my_op PRIVATE
    ${ASCEND_HOME_PATH}/include
    ${ASCEND_HOME_PATH}/include/graph
    ${ASCEND_HOME_PATH}/include/register
    ${ASCEND_HOME_PATH}/include/external
)

# Link libraries (select as needed)
target_link_libraries(my_op PRIVATE
    ascendcl acl_rt ge_compiler ge_executor
)

# Output .so
add_library(my_op SHARED custom_op.cpp)

```

**Additional dependencies for the PyTorch scenario:** TorchNPU, c10, lowering, and others must be linked, and the TorchNPU header file paths must be included. The build artifacts must be installed according to the OPP package specification, as shown below:

```tree
output/                              # ASCEND_CUSTOM_OPP_PATH points to this directory
├── op_graph/
│   ├── include/my_op.h              # proto header file (for use by the graph construction side)
│   └── lib/linux/x86_64/
│       ├── libcust_opapi.so         # deliverable
│       └── my_kernel.cpp            # kernel source code (required for the RTC scenario)
└── bin/
    └── my_kernel.bin                # kernel binary (as needed)
```

For the complete running steps, see [Quick Start](quick_start.md).
