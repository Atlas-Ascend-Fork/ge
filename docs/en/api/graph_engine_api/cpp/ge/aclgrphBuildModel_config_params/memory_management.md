# Memory Management

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:52:46.111Z pushedAt=2026-10-28T07:40:22.212Z nl=2 -->

## EXEC\_DISABLE\_REUSED\_MEMORY

Memory reuse switch. The corresponding **options** parameter of this parameter is `ge.exec.disableReuseMemory`.

Memory reuse refers to reusing non-conflicting memory based on lifecycle and memory size to reduce network memory usage.

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

When generating an om model file, determines whether to externalize the weights of **Const**/**Constant** nodes in the original network and convert the node type to FileConstant. The corresponding **options** parameter for this parameter is `ge.externalWeight`.

In offline scenarios, if the model weights are large and the environment imposes a size limit on the om offline model, it is recommended to enable weight externalization to save the weights separately and reduce the om size.

**Parameter values:**

- **0**: (Default) Weights are not externalized and are directly saved in the om offline model file.
- **1**: Weights are externalized but not normalized. The weight files of all **Const**/**Constant** nodes in the network are written to disk, and the node type is converted to FileConstant. The weight files are saved in the **weight** directory at the same directory as the om file, with the weights of different nodes saved to different files, named **weight\_<hash value\>**.
- 2: Weights are externalized and normalized. The weight files of all **Const**/**Constant** nodes in the network are written to disk, and the node type is converted to FileConstant. The weight files are saved in the **weight** directory at the same directory as the om file, with all weights saved to the same file, named "original graph root graph name \_weight\_combined”.

**Configuration example:**

```c++
{ge::ir_option::EXTERNAL_WEIGHT, "1"}
```

**Constraints:**

- In the weight externalization scenario, when developing inference applications and loading models:

  - Use the [aclgrphSaveModel](../aclgrphSaveModel.md) API to save the om model:
    - If the aclmdlLoadFromFile API is used to load the model, save the weight file in the weight directory at the same level as the om file.
    - If the aclmdlSetConfigOpt and aclmdlLoadWithConfig interfaces are used to load the model, there is no requirement on the weight externalization directory. When loading the model later, specify the weight externalization directory through the aclmdlLoadWithConfig interface.

  - In the weight update scenario, use the [aclgrphBundleSaveModel](../aclgrphBundleSaveModel.md) API to save the om model:

    Only the [aclmdlBundleLoadFromFile](../../../c/acl/aclmdlBundleLoadFromFile.md) API can be used to load the model, and the weight file must be saved in the weight directory at the same level as the om file.

**Applicable products:**

Supported on all chips.
