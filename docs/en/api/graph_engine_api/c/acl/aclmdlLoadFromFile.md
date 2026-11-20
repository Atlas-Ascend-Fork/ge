# aclmdlLoadFromFile

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:35:42.276Z pushedAt=2026-11-18T03:34:47.475Z nl=2 -->

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
<!-- npu="IPV350" id880 -->
- IPV350: not supported
<!-- end id880 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromFile_res.md#id1 -->

## Function Description

Loads offline model data from a file. The system manages the memory used for model execution.

The file passed through the modelPath parameter in this API is a \*.om or .om2 model file. For details about how to obtain an om or .om2 model file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

If the size of the om/om2 model file is restricted, this API also supports loading an external weight file. However, when building the model, the weights must be saved in a separate file. For example, when using the ATC tool to generate an om/om2 file, set the --external\_weight parameter to 1 (1 indicates that the weights of the Const/Constant nodes in the original network are saved in a separate file, which is stored in the weight directory at the same level as the om/om2 file). When using this API to load the om/om2 file, place the weight directory in the same directory as the om/om2 file. This API then automatically searches for the weight file in the weight directory. Otherwise, the separate weight file may fail to load.

## Prototype

```c
aclError aclmdlLoadFromFile(const char *modelPath, uint32_t *modelId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelPath | Input | Pointer to the model file path, which includes the file name. The user running the application (APP) must have access permission to the storage path. |
| modelId | Output | Pointer to the model ID.<br>After the model is loaded successfully, the system returns the model ID, which identifies the model in subsequent operations. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Reference Resources

The [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) and [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) APIs are also available for model loading. They use the properties in the configuration object to distinguish whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by you.
