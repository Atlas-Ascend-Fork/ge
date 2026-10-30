# GE Native Graph Construction for Graph Integration

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:36:51.171Z pushedAt=2026-10-28T03:36:33.831Z nl=2 -->

GE native graph construction is the most direct way to ingest a graph. It requires no framework adaptation layer and directly calls GE APIs to create operator nodes. GE native graph construction has the following characteristics:

- Directly calls APIs for graph construction, with no framework conversion layer.
- The op type is consistent with the REG\_AUTO\_MAPPING\_OP registration name.
- The REG\_OP proto header file must be written manually.

The deliverables involved in each stage are as follows:

1. **Stage 1: Operator Definition**

    In the operator definition stage, metadata such as the inputs, outputs, and attributes of the operator is declared for use in subsequent stages.

    **Implementation methods by frontend framework:**

    | Frontend Framework | Definition Method | Deliverable | Description |
    | --- | --- | --- | --- |
    | GE native | REG_OP header file | op_graph/include/*.h | Manually written proto definition |

    **GE native REG\_OP example:**

    ```c++
    // op_graph/include/add_custom.h
    REG_OP(AddCustom)
        .INPUT(x, TensorType({DT_FLOAT, DT_FLOAT16}))
        .INPUT(y, TensorType({DT_FLOAT, DT_FLOAT16}))
        .OUTPUT(z, TensorType({DT_FLOAT, DT_FLOAT16}))
        .OP_END_FACTORY_REG(AddCustom);
    ```

2. **Stages 2-3: Frontend integration and graph construction**

    There is no framework conversion layer. APIs are called directly to construct the graph, as shown in the following example:

    ```c++
    // Create a custom operator node directly through the GE API
    // "add_node" is the node name, and "AddCustom" is the operator type (consistent with REG_OP)
    auto op = ge::OperatorFactory::CreateOperator("add_node", "AddCustom");
    // Set the operator input ports
    op.SetInput("x", input_x);  // Connect input_x to the x input port of the operator
    op.SetInput("y", input_y);  // Connect input_y to the y input port of the operator
    // Add the operator node to the computation graph
    graph.AddOp(op);
    ```

    The op type used on the graph construction side (AddCustom in this example) must be consistent with REG\_OP and REG\_AUTO\_MAPPING\_OP

3. **Stage 4: Graph compilation**

    The callback interfaces for graph compilation are triggered during GE compilation and are used for shape inference, kernel compilation, and serialization. **Callback interfaces and trigger timing:**

    | Callback Interface | Trigger Time | Applicable Scenarios |
    | --- | --- | --- |
    | InferShape/InferDataType | Shape/DataType inference at compile time | All scenarios (optional) |
    | Compile | RTC kernel compilation at compile time | Online compilation/offline OM scenarios |
    | Serialize | Write to OM during ATC compilation | Offline OM scenarios |

    For detailed examples of the callback interfaces, see [Callback Interface](callback_interface.md).

4. **Stage 5: Graph execution**

    This stage is the core of a custom operator. GE calls back the Execute interface during execution.

    **Callback interfaces and trigger timing:**

    | Callback Interface | Trigger Time | Applicable Scenarios |
| --- | --- | --- |
| Execute | Execute the kernel at runtime | All scenarios (must be implemented) |
| Deserialize | Restore the binary when the OM is loaded | Offline OM scenario |
| UpdateHostArgs | Refresh when the I/O address changes | Static graph offloading scenario |

    For detailed examples of the callback interfaces, see [Callback Interface](callback_interface.md).

**Summary: GE native composition stages and deliverables**

| Stage | Deliverable | Description |
| --- | --- | --- |
| Stage 1: Operator definition | op_graph/include/*.h | REG_OP proto header file |
| Stage 2: Frontend access | None | Direct API call |
| Stage 3: Graph construction | None | Direct graph construction |
| Stage 4: Graph compilation | op_graph/lib/<os\>/<arch\>/*.so | The callback is implemented in the GE deliverable |
| Stage 5: Graph execution | op_graph/lib/<os\>/<arch\>/*.so | The callback is implemented in the GE deliverable |
