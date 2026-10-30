# CustomPassStage

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:17:42.858Z pushedAt=2026-10-28T07:40:22.145Z nl=2 -->

Custom pass execution stage. Header file: \#include <register/register\_custom\_pass.h\>

```c++
enum class CustomPassStage : uint32_t {
  kBeforeInferShape = 0,
  kAfterInferShape = 1,
  kAfterAssignLogicStream = 2, // only support CustomAllocateStreamPassFunc in this stage
  kAfterBuiltinFusionPass = 3,
  kAfterOriginGraphOptimize=4,
  kCompatibleInherited=5,
  kInvalid
};
```

- **kBeforeInferShape**: (Default value) The custom pass is executed before **InferShape** at the framework entry.
- **kAfterInferShape**: The custom pass is executed after **InferShape**.

    If the custom pass is executed after **InferShape**, the pass must ensure the continuity of shapes after the graph is modified:

    ```c++
        // 1. Obtain the output description of the input node node1.
        TensorDesc output_desc;
        node1.GetOutputDesc(0, output_desc);
        // 2. Update the input description of the current node node2.
        node2.UpdateInputDesc(0, output_desc);
        // 3. Perform InferShape on the current node node2.
        operator2.InferShapeAndType();
    ```

    When the **InferShape** function is called, the original shape of the input is written to the operator's shape before **InferShape**, and the operator's output shape is written to the original shape of the operator's output after **InferShape**. Therefore, when setting **InputDesc** for an operator, the original shape must be set.

- **kAfterAssignLogicStream**: The custom pass is executed after the logical stream assignment stage. This stage accepts only passes for logical stream assignment (to register a custom logical stream assignment pass execution function, see [CustomAllocateStreamPassFn](./PassRegistrationData/CustomAllocateStreamPassFn.md)). Because graph modification is not allowed in this stage, graph-modifying passes in other scenarios will be validated and report an error.
- **kAfterBuiltinFusionPass**: The custom pass is executed after the built-in original graph fusion pass.
- **kAfterOriginGraphOptimize**: The custom pass is executed after the original graph optimization phase.
- **kCompatibleInherited**: Applies only to built-in passes. Users do not need to register this field. Registering this field for a custom pass has no effect.

    > [!NOTE]
    > Passes registered to the **kAfterOriginGraphOptimize** and **kCompatibleInherited** stages must call the [CheckNodeSupportOnAicore](./GeUtils/CheckNodeSupportOnAicore.md) API on new nodes to verify whether the new nodes support execution on the AI Core.

- **kInvalid**: No effect.
