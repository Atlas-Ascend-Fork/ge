# aclmdlLoadFromMemWithQ

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:37:23.476Z pushedAt=2026-11-18T03:35:16.922Z nl=2 -->

## Product Support

<!-- npu="950" id664 -->
- Ascend 950PR&950DT products: Supported
<!-- end id664 -->
<!-- npu="A3" id665 -->
- Atlas A3 products: Supported
<!-- end id665 -->
<!-- npu="910b" id666 -->
- Atlas A2 products: Supported
<!-- end id666 -->
<!-- npu="310b" id667 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id667 -->
<!-- npu="310p" id668 -->
- Atlas inference products: Supported
<!-- end id668 -->
<!-- npu="910" id669 -->
- Atlas training products: Supported
<!-- end id669 -->
<!-- npu="IPV350" id670 -->
- IPV350: not supported
<!-- end id670 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlLoadFromMemWithQ_res.md#id1 -->

## Function Description

Loads OM model file data from memory. Both the input and output data of the model are stored in queues. This API supports only models with fixed-shape inputs.

For details about how to obtain an OM model file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

## Prototype

```c
aclError aclmdlLoadFromMemWithQ(const void *model, size_t modelSize, uint32_t *modelId, const uint32_t *inputQ, size_t inputQNum, const uint32_t *outputQ, size_t outputQNum)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Memory address pointer to the model data. |
| modelSize | Input | Length of the model data in memory, in bytes. |
| modelId | Output | Pointer to the model ID.<br>After the model is loaded successfully, the system returns the model ID, which identifies the model in subsequent operations. |
| inputQ | Input | Pointer to the queue ID. Each model input corresponds to one queue ID. |
| inputQNum | Input | Input queue size. |
| outputQ | Input | Pointer to the queue ID. Each model output corresponds to one queue ID. |
| outputQNum | Input | Output queue size. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Reference Resources

The [aclmdlSetConfigOpt](aclmdlSetConfigOpt.md) and [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) APIs are also available for model loading. They use the properties in the configuration object to distinguish whether the model is loaded from a file or from memory, and whether the memory is managed internally by the system or by you.
