# aclmdlLoadFromFile

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:27:13.792Z pushedAt=2026-10-29T03:24:00.929Z nl=2 -->

## Product Support

<!-- npu="950" id874 -->
- Ascend 950PR&950DT products: Supported
<!-- end id874 -->
<!-- npu="A3" id875 -->
- Atlas A3 products: Supported
<!-- end id875 -->
<!-- npu="910b" id876 -->
- Atlas A2 products: Supported
<!-- end id876 -->
<!-- npu="310b" id877 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id877 -->
<!-- npu="310p" id878 -->
- Atlas inference products: Supported
<!-- end id878 -->
<!-- npu="910" id879 -->
- Atlas training products: Supported
<!-- end id879 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromFile_res.md#id1 -->

## Feature Description

Loads offline model data from a file. The system manages the memory used for model execution.

The file passed through the modelPath parameter in this API is a \*.om model file. For details about how to obtain an om model file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

If the size of the om model file is limited, this API also supports loading an external weight file. However, when building the model, you must save the weights in a separate file. For example, when using the ATC tool to generate an om file, set the --external\_weight parameter to 1 (1 means that the weights of the Const/Constant nodes in the original network are saved in a separate file, and the file is saved in the weight directory at the same level as the om file). Then, when using this API to load the om file, place the weight directory in the same directory as the om file. The API automatically searches for the weight file in the weight directory; otherwise, the separate weight file may fail to load.

## Prototype

```c
aclError aclmdlLoadFromFile(const char *modelPath, uint32_t *modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelPath | Input | Pointer to the model file path, which includes the file name. The user running the application (APP) must have access permission to this storage path. |
| modelId | Output | Pointer to the model ID.<br>After the model is loaded successfully, the system returns the model ID, which serves as the identifier for the model in subsequent operations. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## References

The [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) and [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) APIs are also available for model loading. They use properties in the configuration object to distinguish whether the model is loaded from a file or from memory, and whether the memory is managed by the system or by you.
