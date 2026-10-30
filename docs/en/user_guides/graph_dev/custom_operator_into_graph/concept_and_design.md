# Concepts and Design

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:34:09.280Z pushedAt=2026-10-28T03:30:34.499Z nl=2 -->

## Interface System

All custom operator implementation classes must inherit from BaseCustomOp and flexibly combine the following capability interfaces as required. Each interface corresponds to an independent callback timing, and GE invokes the corresponding callback at a **specific stage of graph compilation or execution**.

```tree
BaseCustomOp (common base class)
├── EagerExecuteOp     → Execute(ctx)          Runtime execution
├── ShapeInferOp       → InferShape(ctx)       Shape inference
│                        → InferDataType(ctx)    Data type inference
├── CompilableOp       → Compile(ctx)          Online compilation
├── PortableOp         → Serialize(buf)        Serialize to OM
│                        → Deserialize(buf)      Deserialize from OM
└── ArgsUpdater        → UpdateHostArgs(ctx)   I/O address refresh
```

For details about the APIs, see [Callback Interface](callback_interface.md).

Example implementation class:

```c++
// Inherit EagerExecuteOp to obtain runtime execution capability.
class MyCustomOp : public EagerExecuteOp {
  graphStatus Execute(gert::EagerOpExecutionContext *ctx) override { /* ... */ }
};

// Register to GE: instantiate this class and call back Execute when a node of the "MyCustomOp" type is encountered in the graph.
REG_AUTO_MAPPING_OP(MyCustomOp);
```

## Registration Mechanism

Custom operators require two types of registration:

- **REG\_OP**: **Declare the operator prototype**

    Define the input, output, and attribute protos of the operator for use when creating nodes on the composition side:

    ```c++
    // Register the GE operator prototype and declare the operator inputs, outputs, and supported data types.
    // MyCustomOp is the operator type name and must be consistent with the class name in the subsequent REG_AUTO_MAPPING_OP.
    REG_OP(MyCustomOp)
        .INPUT(x, TensorType({DT_FLOAT, DT_FLOAT16}))    // Input x
        .INPUT(y, TensorType({DT_FLOAT, DT_FLOAT16}))    // Input y
        .OUTPUT(z, TensorType({DT_FLOAT, DT_FLOAT16}))   // Output z
        .OP_END_FACTORY_REG(MyCustomOp);
    ```

- **REG\_AUTO\_MAPPING\_OP**: **Map the implementation class**

    Map the C++ implementation class to the op type. GE looks up and instantiates the class by the op type name:

    ```c++
    // Custom algorithm implementation class that inherits EagerExecuteOp to obtain runtime execution capability
    class MyCustomOp : public EagerExecuteOp { /* ... */ };
    // Register the implementation class with GE. When GE encounters a node of the "MyCustomOp" type in the graph, it instantiates this class and calls back its Execute method
    REG_AUTO_MAPPING_OP(MyCustomOp);
    ```

    > [!NOTE]
    >**Naming constraint**: The op type name in REG\_OP, the class name in REG\_AUTO\_MAPPING\_OP, and the op type used on the composition side must be identical.

- **Two Shape inference paradigms (choose one)**

  | Paradigm | Syntax | Applicable |
  | --- | --- | --- |
  | Inherit the interface | The implementation class inherits ShapeInferOp and overrides InferShape(ctx) | Used in combination with interfaces such as EagerExecuteOp, in an object-oriented style. |
  | Register a function | IMPL_OP(MyCustomOp).InferShape(InferShapeFn) | Decoupled from the implementation class, with the inference function registered independently. |

    Either approach is acceptable; choose one and apply it consistently throughout the project. The only difference is whether the inference logic is encapsulated inside the implementation class or registered as an independent function.

## Deliverables and OPP Package

A custom operator is loaded by GE in the form of a .so deliverable, with its path specified through the ASCEND\_CUSTOM\_OPP\_PATH environment variable. For the recommended OPP package directory structure and .so naming and deployment steps, see [Deliverables and OPP Specifications](deliverables_and_opp.md).

Configuration method:

```bash
export ASCEND_CUSTOM_OPP_PATH="<custom_opp_root>:$ASCEND_CUSTOM_OPP_PATH"
```

## Execution Scenarios

The development of a custom operator requires implementing different interface combinations based on the specific scenario. This section describes typical application scenarios and their development key points. The **execution scenario** decision table is as follows:

| Scenario | Description | Interface Combination | Typical Link | Reference Example |
| --- | --- | --- | --- | --- |
| Scenario 1: Dynamic graph online execution | The kernel is precompiled, and Execute directly launches it. | EagerExecuteOp+ShapeInferOp (optional) | Composition → Direct execution | [ascendc_add_custom](../../../../../examples/custom_op/ascendc_add_custom/README.md) |
| Scenario 2: Online compilation + online execution | The kernel is compiled by RTC during graph compilation. | EagerExecuteOp+CompilableOp+ShapeInferOp (optional) | Composition → Compile → Execute | - |
| Scenario 3: Offline OM model offloading | The compilation artifacts are serialized into the OM and restored after loading. | EagerExecuteOp+CompilableOp+ShapeInferOp+PortableOp | AIR → ATC → OM → ACL | [compilable_add_custom](../../../../../examples/custom_op/compilable_add_custom/README.md) |
| Scenario 4: Data-dependent shape | The output shape depends on runtime data and is written back to the shape buffer. | EagerExecuteOp+ShapeInferOp | Composition → Execute + shape buffer write-back | [data_dependent_shape_custom](../../../../../examples/custom_op/data_dependent_shape_custom/README.md) |

- **Scenario 1: Dynamic graph online execution**

    The simplest scenario. The kernel is already precompiled (Ascend C .asc compiled with the same library, Triton .npubin, etc.), and only needs to be loaded and launched in Execute.

    ```plaintext
    Composition → GE callback Execute → Obtain input → Allocate output → Launch kernel → Return
    ```

    **Key point:**

  - Only EagerExecuteOp needs to be implemented.
  - ShapeInferOp is optional: if the output shape is the same as the input and REG\_OP has been declared, it can be omitted.
  - The kernel binary is compiled together with the .so, or loaded from a file in Execute.

- **Scenario 2: Online compilation + online execution**

    The kernel source code needs to be compiled into a device binary through RTC (Runtime Compilation) during the GE compilation phase.

    ```plaintext
    Composition → GE callback Compile (RTC compiles the kernel)
         → GE callback Execute (loads the binary and launches the kernel)
    ```

    **Key point:**

  - In the Compile callback, the complete metadata such as the input shape, dtype, and format can be obtained through ctx-\>GetInputTensor\(i\).
  - It is recommended to generate a binary key based on the shape to support management of multiple shapes and multiple kernels.
  - The compilation artifacts are cached in the member variables of the implementation class for use by Execute.

- **Scenario 3: Offline OM model deployment**

    Based on Scenario 2, the compilation artifacts need to be serialized into the OM model file, and deserialized to restore them when the OM is subsequently loaded.

    ```plaintext
    Composition → ATC offline compilation → Callback Compile (RTC compilation)
                         → Callback Serialize (write binary to OM)
         → acl loads OM → Callback Deserialize (restore binary from OM)
                         → Callback Execute (launch kernel)
    ```

    **Key points:**

  - PortableOp must be implemented. The buffer format for Serialize/Deserialize is user-defined; GE only passes it through without parsing.
  - ShapeInferOp is called during OM compilation to infer the output shape/dtype.
  - Serialization of multiple binaries is supported (managed by key).

- **Scenario 4: Data-dependent shape (three types of operators)**

    The output shape depends on runtime input data (such as Where and NonZero), and only the upper bound can be determined at compile time.

    ```plaintext
    InferShape → upper bound of output shape (including unknown dimensions)
    Execute    → allocate output by upper bound + allocate shape buffer
               → launch kernel (kernel writes the actual shape back to the shape buffer)
               → stream sync → D2H read back shape buffer → update output shape
    ```

    **Key points:**

  - In InferShape, use -1 to indicate an unknown dimension.
  - In Execute, allocate the output by the maximum shape and additionally allocate a shape buffer (through MallocWorkSpace).
  - After kernel execution, synchronize the stream and read back the shape buffer on your own.
  - Shape buffer format: \[ndim, d0, d1, ...\] (uint64\_t array).

## Broadcast Semantics Example

The output shape derivation of the Add operator follows the broadcast rule: align dimensions from the tail; if a dimension is 1, expand it to match the other side; if they are equal, keep it; otherwise, the case is considered invalid.

```c++
graphStatus InferShapeForAdd(gert::InferShapeContext *context) {
  const auto &in0 = *context->GetInputShape(0);
  const auto &in1 = *context->GetInputShape(1);
  auto *out = context->GetOutputShape(0);

  size_t dims0 = in0.GetDimNum();
  size_t dims1 = in1.GetDimNum();
  size_t out_dims = std::max(dims0, dims1);
  out->SetDimNum(out_dims);

  for (size_t i = 0; i < out_dims; ++i) {
    // Tail alignment: dimensions beyond the operator's own rank are treated as dimension 1
    int64_t d0 = (i + dims0 >= out_dims) ? in0.GetDim(i - (out_dims - dims0)) : 1;
    int64_t d1 = (i + dims1 >= out_dims) ? in1.GetDim(i - (out_dims - dims1)) : 1;

    if (d0 == d1 || d0 == 1 || d1 == 1) {
      out->SetDim(i, std::max(d0, d1));   // Expand to the other side's dimension when the dimension is 1
    } else {
      // Invalid broadcast: for example, (2) and (3) cannot be reduced by taking the max dimension by dimension
      return GRAPH_FAILED;
    }
  }
  return GRAPH_SUCCESS;
}

IMPL_OP(AddCustom).InferShape(InferShapeForAdd);
```
