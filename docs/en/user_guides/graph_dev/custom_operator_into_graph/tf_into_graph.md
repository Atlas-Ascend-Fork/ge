# TensorFlow Graph Integration

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:44:52.315Z pushedAt=2026-10-28T06:03:05.595Z nl=2 -->

The main difference between the TensorFlow scenario and the PyTorch scenario is that the Adapter can automatically generate GE operator prototypes from the TF op\_def, so developers do not need to manually write REG\_OP. A brief diagram of the deliverables involved on both sides is shown below. For a TensorFlow scenario example, see [Triton Graph Integration](../appendix/triton_into_graph.md) or [triton\_add\_custom](../../../../../examples/custom_op/triton_add_custom).

![](../figures/tf_into_graph.png)

The following describes each stage and its deliverables in detail.

1. **Stage 1: Operator Definition**

    In the operator definition stage, metadata such as the inputs, outputs, and attributes of an operator is declared for use in subsequent stages.

    | Frontend Framework | Definition Method | Deliverable | Description |
    | --- | --- | --- | --- |
    | TensorFlow | TF op_def | libcustom_ops.so | Operator declaration on the TF side; GE automatically generates REG_OP |

    **Example of operator registration on the TensorFlow side (using TF 1.15 as an example):**

    ```c++
    #include "tensorflow/core/framework/op.h"
    #include "tensorflow/core/framework/op_kernel.h"
    #include "tensorflow/core/framework/shape_inference.h"
    #include "tensorflow/core/framework/common_shape_fns.h"

    using namespace tensorflow;

    // Register the operator prototype through the REGISTER_OP API provided by TensorFlow.
    // After registration, the GE Adapter can automatically generate the GE operator prototype from this definition, so there is no need to manually write REG_OP.
    REGISTER_OP("AddCustom")
        .Input("x: T")
        .Input("y: T")
        .Output("z: T")
        .Attr("T: {float, double, int32}")
        .SetShapeFn(shape_inference::BroadcastBinaryOpShapeFn);

    // CPU implementation of the TensorFlow custom operator (CPU is not supported; declaration only).
    // The actual computation is performed on the NPU through GE graph execution.
    class AddCustomOp : public OpKernel {
    public:
        explicit AddCustomOp(OpKernelConstruction* context) : OpKernel(context) {}
        void Compute(OpKernelContext* context) override {
            OP_REQUIRES(context, false, errors::Unimplemented("AddCustomOp is not supported on CPU"));
        }
    };

    // Register the CPU kernel implementation (required; otherwise TF cannot load the operator).
    REGISTER_KERNEL_BUILDER(Name("AddCustom").Device(DEVICE_CPU), AddCustomOp);
    ```

2. **Stage 2: Frontend Integration**

    Map the frontend operator type to the GE operator type and handle attribute conversion.

    | Frontend Framework | Integration Method | Deliverables Involved | Key Point |
    | --- | --- | --- | --- |
    | TensorFlow | Automatic mapping by Adapter | npu_supported_ops.json | Configure operator visibility in JSON |

    **For the TensorFlow scenario:**

    1. Custom operator .so on the TensorFlow side (declares operator visibility).
    2. Deliverable .so on the GE side (only REG\_AUTO\_MAPPING\_OP is required; no additional REG\_OP needs to be provided).
    3. npu\_supported\_ops.json (in the TensorFlow scenario, automatic mapping is performed by the Adapter, and this deliverable is a key deliverable).

    **npu\_supported\_ops.json Example:**

    ```json
    {
      "custom_ops": [
        {
          "op_name": "AddCustom",
          "op_type": "AddCustom"
        }
      ]
    }
    ```

    **Deliverable organization example:**

    ```tree
    custom_opp/
    ├── op_graph/
    │   └── lib/linux/x86_64/libcust_opapi.so  # GE deliverable (only REG_AUTO_MAPPING_OP is required)
    ├── framework/
    │   └── tensorflow/
    │       ├── libcustom_ops.so               # TF-side operator declaration
    │       └── npu_supported_ops.json         # Adapter configuration
    ```

    For the unified deliverable specifications, see [Deliverables and OPP Specifications](deliverables_and_opp.md).

3. **Stage 3: Graph construction and integration**

    In the graph construction and integration stage, the frontend computation graph is converted into a GE computation graph.

    | Frontend framework | Graph input path | Conversion mechanism |
    | --- | --- | --- |
    | TensorFlow | TF Graph → GE Graph | Adapter |

    This stage is automatically completed by the framework, and no additional deliverables are required from developers. After the graph input is completed, the GE graph already contains the custom operator nodes.

4. **Stage 4: Graph Compilation**

    The callback interfaces for graph compilation are triggered during GE compilation and are used for shape inference, kernel compilation, and serialization. **Callback interfaces and trigger timing:**

    | Callback Interface | Trigger Time | Applicable Scenarios |
    | --- | --- | --- |
    | InferShape/InferDataType | Shape/DataType inference at compile time | All scenarios (optional) |
    | Compile | RTC kernel compilation at compile time | Online compilation/offline OM scenario |
    | Serialize | Writing to OM during ATC compilation | Offline OM scenario |

    For detailed examples of the callback interfaces, see [Callback Interface](callback_interface.md).

5. **Stage 5: Graph Execution**

    This stage is the core of a custom operator. GE calls back the Execute interface during execution.

    **Callback interfaces and trigger timing:**

    | Callback Interface | Trigger Time | Applicable Scenarios |
    | --- | --- | --- |
    | Execute | Kernel execution at runtime | All scenarios (must be implemented) |
    | Deserialize | Restoring binary when OM is loaded | Offline OM scenario |
    | UpdateHostArgs | Refreshing when I/O addresses change | Static graph offloading scenario |

    For detailed examples of the callback interfaces, see [Callback Interface](callback_interface.md).

**Summary: TensorFlow Graph Integration Stages and Deliverables**

| Stage | Deliverables Involved | Description |
| --- | --- | --- |
| Stage 1: operator definition | framework/tensorflow/libcustom_ops.so | TF-side operator declaration |
| Stage 2: frontend integration | framework/tensorflow/npu_supported_ops.json | Adapter configuration |
| Stage 3: graph construction and integration | None | Automatic conversion by Adapter |
| Stage 4: graph compilation | op_graph/lib/<os\>/<arch\>/*.so | Callback implemented in GE deliverables |
| Stage 5: graph execution | op_graph/lib/<os\>/<arch\>/*.so | Callback implemented in GE deliverables |
