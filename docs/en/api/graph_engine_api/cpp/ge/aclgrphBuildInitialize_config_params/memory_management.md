# Memory Management

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:33:37.349Z pushedAt=2026-10-28T07:40:22.193Z nl=2 -->

## EXEC\_DISABLE\_REUSED\_MEMORY

Memory reuse switch. The corresponding **options** parameter for this parameter is `ge.exec.disableReuseMemory`.

Memory reuse refers to reusing non-conflicting memory based on the lifecycle and memory size to reduce network memory usage.

**Parameter values:**

- **0**: (Default value) Enables memory reuse.
- **1**: Disables memory reuse. If the network model is large, disabling the memory reuse switch causes the on-device memory not to be reused during subsequent inference, which may lead to insufficient memory.

**Configuration example:**

```c++
{ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, "0"}
```

**Applicable products:**

Supported on all chips.

## EXTERNAL\_WEIGHT

When generating an OM model file, specify whether to externalize the weights of **Const**/**Constant** nodes in the original network and convert the node type to the FileConstant type. The **options** parameter actually corresponding to this parameter is `ge.externalWeight`.

In offline scenarios, if the model weights are large and the environment imposes a size limit on the OM offline model, it is recommended to enable weight externalization to save the weights separately and reduce the OM size.

**Parameter values:**

- **0**: (Default value) The weights are not externalized and are directly saved in the OM offline model file.
- **1**: The weights are externalized but not normalized. The weight files of all **Const**/**Constant** nodes in the network are written to disk, and the node type is converted to the FileConstant type. The **weight** files are saved under the weight directory at the same level as the OM file, and the weights of different nodes are saved to different files, named with **weight\_<hash value\>**.
- **2**: The weights are externalized and normalized. The weight files of all **Const**/**Constant** nodes in the network are written to disk, and the node type is converted to the FileConstant type. The weight files are saved under the **weight** directory at the same level as the OM file, and all weights are saved to the same file, named with **"original graph root graph name \_weight\_combined"**.

**Configuration example:**

```c++
{ge::ir_option::EXTERNAL_WEIGHT, "1"}
```

**Constraints:**

- In the weight externalization scenario, when developing inference applications and loading models:

  - Use the [aclgrphSaveModel](../aclgrphSaveModel.md) API to save the OM model:
    - If the aclmdlLoadFromFile API is used to load the model, save the weight file in the weight directory at the same level as the OM file.
    - If the aclmdlSetConfigOpt and aclmdlLoadWithConfig APIs are used to load the model, there is no requirement on the weight externalization directory. When loading the model later, specify the weight externalization directory through the aclmdlLoadWithConfig API.

  - In the weight update scenario, use the [aclgrphBundleSaveModel](../aclgrphBundleSaveModel.md) API to save the OM model:

    Only the [aclmdlBundleLoadFromFile](../../../c/acl/aclmdlBundleLoadFromFile.md) API can be used to load the model, and the weight file must be saved in the weight directory at the same level as the OM file.

**Applicable products:**

Supported on all chips.
