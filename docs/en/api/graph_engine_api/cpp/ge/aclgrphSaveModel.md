# aclgrphSaveModel

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:12:29.621Z pushedAt=2026-10-28T07:40:22.130Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_ir\_build.h\>
- Library file: libge\_compiler.so

## Description

Serializes the offline model and saves it to a specified file.

## Prototype

> [!NOTE]
> The string-type API will be deprecated in future versions. You are advised to use the non-string type instead.

```c++
graphStatus aclgrphSaveModel(const std::string &output_file, const ModelBufferData &model)
graphStatus aclgrphSaveModel(const char_t *output_file, const ModelBufferData &model)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Buffer data of the offline model. For details, see [ModelBufferData](ModelBufferData.md).<br>**data** points to the generated model data, and **length** indicates the actual size of the model. |
| output_file | Input | Name of the file for saving the offline model. The generated offline model file name automatically ends with the .om suffix, for example, **ir_build_sample.om** or<br>**ir_build_sample_linux_x86_64.om**. If the om file name contains the operating system and architecture, the om file can be used only in the running environment of that operating system and architecture. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Constraints

If the generated om model file name contains the operating system and architecture, but the operating system and its architecture are inconsistent with the model running environment, use the file name together with the **OPTION\_HOST\_ENV\_OS** and **OPTION\_HOST\_ENV\_CPU** parameters to set the operating system type and architecture of the model running environment. For details about the parameters, see [Configuration Parameters Supported by aclgrphBuildInitialize](./aclgrphBuildInitialize_config_params/basic_functions.md).
