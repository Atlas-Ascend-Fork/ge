# GetCompiledModel

<!-- md-trans-meta sourceCommit=ba12e592e8f84bc59709464bd3ad6ca010498d8b translatedAt=2026-10-16T13:49:40.484Z pushedAt=2026-10-28T07:40:22.655Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Obtains the serialized model after graph compilation.

Before calling this API, you must first call [CompileGraph](./CompileGraph.md). After obtaining the serialized model data **ModelBufferData** in the memory buffer through this API, the subsequent operations are divided into two types:

- Usage 1: (Recommended) Generate an om offline model
    1. Online compilation: Obtain the compiled serialized model through the **GetCompiledModel** API.
    2. Call the [aclgrphSaveModel](../aclgrphSaveModel.md) API to save the serialized model as an om offline model.

        When calling the **GetCompiledModel** API and the **aclgrphSaveModel** API, you must ensure that the CANN software package versions used are **consistent**. No validation is performed within the APIs.

    3. Call the **aclmdlLoadFromFile** API to load the model **from a file**, and then use the **aclmdlExecute** API to perform inference.

- Usage 2: Do not generate an om offline model.
    1. Online compilation: Obtain the compiled serialized model through the GetCompiledModel API.
    2. Call the **load from memory** model API aclmdlLoadFromMem, and then use the aclmdlExecute API to perform inference.

Usage 1 is recommended: After an om offline model is generated, the CANN software package version numbers used to call the **GetCompiledModel** API and the **aclmdlLoadFromFile** API **may be inconsistent**. However, if Usage 2 is used, because the serialized model data **ModelBufferData** in the memory buffer does not guarantee version compatibility, the CANN software package version numbers used to call the **GetCompiledModel** API and the **aclmdlLoadFromMem** API must be **consistent**; otherwise, undefined behavior may occur.

For details about the APIs, see the "Model Inference" section in [Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md).

## Prototype

```c++
Status GetCompiledModel(uint32_t graph_id, ModelBufferData &model_buffer)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the corresponding graph. |
| model_buffer | Output | Serialized model stored in the memory buffer. For details, see [ModelBufferData](../ModelBufferData.md).<br>Here, **data** points to the generated model data, and **length** indicates the actual size of the model. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Success.<br>**FAILED**: Failure. |

## Constraints

- Fusion of variables and conversion operators causes the graph to be recompiled: Because the serialized model obtained through the **GetCompiledModel** API cannot be compiled again, if the graph contains variables, set the [performance tuning](../options_params/overall_constraints.md)  \> **ge.exec.variable\_acc** parameter to **False** to disable the fusion of variables and conversion operators.
- For weight file management after enabling the external weight feature ([memory management](../options_params/memory_management.md)  \>**ge.externalWeight**), note the following points:
    - Default behavior: When the external weight feature is enabled, the system writes the weight file to a temporary directory by default (for the specific path, see the ge.externalWeight description). Note that when the GeSession object is destructed, the temporary directory and the files under it are automatically cleaned up and deleted.
    - Recommended operation: To prevent weight files from being lost after **GeSession** is destroyed, you can use either of the following methods to persist them:

        - Specify a dedicated directory: During initialization, configure the `ge.externalWeightDir` parameter to directly specify a dedicated, non-temporary path for storing weight files.
        - Back up files in advance: Before the **GeSession** object is destroyed, have the application proactively copy the weight files from the temporary path to another specified safe directory.
        - Save the offline model: Before **GeSession** is destroyed, call the `aclgrphSaveModel` API to serialize the compiled model (including weights) and save it as an independent om offline model file.

        Using the methods above, you can effectively manage the lifecycle of external weight files and ensure that they are not accidentally lost when **GeSession** is destroyed.
