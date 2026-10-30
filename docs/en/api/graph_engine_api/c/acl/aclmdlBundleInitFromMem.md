# aclmdlBundleInitFromMem

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:54:03.400Z pushedAt=2026-10-29T02:55:15.388Z nl=2 -->

## Product Support

<!-- npu="950" id636 -->
- Ascend 950PR&950DT products: Supported
<!-- end id636 -->
<!-- npu="A3" id637 -->
- Atlas A3 products: Supported
<!-- end id637 -->
<!-- npu="910b" id638 -->
- Atlas A2 products: Supported
<!-- end id638 -->
<!-- npu="310b" id639 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id639 -->
<!-- npu="310p" id640 -->
- Atlas inference products: Supported
<!-- end id640 -->
<!-- npu="910" id641 -->
- Atlas training products: Supported
<!-- end id641 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleInitFromMem_res.md#id1 -->

## Feature Description

During the model execution phase, if the scenario involves dynamically updating variables, you can call this API to initialize the model from memory.

## Prototype

```c
aclError aclmdlBundleInitFromMem(const void* model, size_t modelSize, void *varWeightPtr, size_t varWeightSize, uint32_t *bundleId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Memory address pointer to the model data, managed by you. Do not release this memory during aclmdlBundleLoadModel.<br>The model file is built based on the graph building API. Call aclgrphBundleBuildModel to compile the model, call aclgrphBundleSaveModel to save the model, and then read the saved .om model file into memory. For details about the graph building API, see [aclgrphBundleBuildModel](../../cpp/ge/aclgrphBundleBuildModel.md). |
| modelSize | Input | Length of the model data in memory, in bytes. |
| varWeightPtr | Input | Address pointer to the refreshable weight memory required by the model, managed by you. Do not release this memory during model execution.<br>If a null pointer is passed, the memory is managed by the system. |
| varWeightSize | Input | Size of the refreshable weight memory required by the model, in bytes. This parameter is invalid when varWeightPtr is a null pointer. |
| bundleId | Output | After the system successfully initializes the bundled model, bundleId is returned as the identifier for recognizing the model in subsequent operations. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
In Ascend EP mode, the memory for the model parameter must be allocated on the Host.
<!-- end id1 -->

<!-- npu="310b" id2 -->
In Ascend RC mode, the memory for the model parameter must be allocated on the Device.
<!-- end id2 -->

<!-- npu="310p" id3 -->
In Control CPU open mode, the memory for the model parameter must be allocated on the Device.
<!-- end id3 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleInitFromMem_res.md#id2 -->
