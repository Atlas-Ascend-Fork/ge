# aclmdlLoadFromFileWithQ

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:27:31.477Z pushedAt=2026-10-29T03:31:24.345Z nl=2 -->

## Product Support

<!-- npu="950" id951 -->
- Ascend 950PR&950DT products: Supported
<!-- end id951 -->
<!-- npu="A3" id952 -->
- Atlas A3 products: Supported
<!-- end id952 -->
<!-- npu="910b" id953 -->
- Atlas A2 products: Supported
<!-- end id953 -->
<!-- npu="310b" id954 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id954 -->
<!-- npu="310p" id955 -->
- Atlas inference products: Supported
<!-- end id955 -->
<!-- npu="910" id956 -->
- Atlas training products: Supported
<!-- end id956 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromFileWithQ_res.md#id1 -->

## Feature Description

Loads offline model data from a file, with both the model input and output data stored in queues. This API supports loading only models with fixed-shape inputs.

The file passed through the modelPath parameter in this API is a \*.om model file. For details about how to obtain an om model file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

If the om model file size is limited, this API also supports loading an external weight file. However, when building the model, you must save the weights in a separate file. For example, when using the ATC tool to generate an om file, set the --external\_weight parameter to 1 (1 means that the weights of the Const/Constant nodes in the original network are saved in a separate file, and the file is saved in the weight directory at the same level as the om file). Then, when using this API to load the om file, place the weight directory at the same level as the om file. This API automatically searches for the weight file in the weight directory. Otherwise, the separate weight file may fail to load.

## Prototype

```c
aclError aclmdlLoadFromFileWithQ(const char *modelPath, uint32_t *modelId, const uint32_t *inputQ, size_t inputQNum, const uint32_t *outputQ, size_t outputQNum)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelPath | Input | Pointer to the model file path, which includes the file name. The user running the application (APP) must have access permission to the storage path. |
| modelId | Output | Pointer to the model ID.<br>After the model is loaded successfully, the system returns the model ID, which serves as the identifier for the model in subsequent operations. |
| inputQ | Input | Pointer to the queue ID. Each input of a model corresponds to one queue ID. |
| inputQNum | Input | Input queue size. |
| outputQ | Input | Pointer to the queue ID. Each output of a model corresponds to one queue ID. |
| outputQNum | Input | Output queue size. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## References

The [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) API and the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API are also available for model loading. They are distinguished by the attributes in the configuration object, which determine whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by you.
