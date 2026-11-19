# aclmdlBundleInitFromMem

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T13:57:15.370Z pushedAt=2026-11-18T03:17:53.202Z nl=2 -->

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
<!-- npu="IPV350" id642 -->
- IPV350: not supported
<!-- end id642 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleInitFromMem_res.md#id1 -->

## Function Description

During the execution of an om or om2 model, if dynamic variable update is involved, call this API to initialize the om/om2 model in memory.

## Prototype

```c
aclError aclmdlBundleInitFromMem(const void* model, size_t modelSize, void *varWeightPtr, size_t varWeightSize, uint32_t *bundleId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Memory address pointer to the model data, managed by you. Do not release this memory during aclmdlBundleLoadModel.<br>The model file is built based on the graph building API. Call aclgrphBundleBuildModel to compile the model, call aclgrphBundleSaveModel to save the model, and then read the saved om/om2 model file into memory. For details about the graph building API, see [aclgrphBundleBuildModel](../../cpp/ge/aclgrphBundleBuildModel.md). |
| modelSize | Input | Length of the model data in memory, in bytes. |
| varWeightPtr | Input | Address pointer to the refreshable weight memory required by the model, managed by you. Do not release this memory during model execution.<br>If a null pointer is passed, the memory is managed by the system. |
| varWeightSize | Input | Size of the refreshable weight memory required by the model, in bytes. This parameter is invalid when varWeightPtr is a null pointer. |
| bundleId | Output | After the system successfully initializes the bundled model, bundleId is returned as the identifier for recognizing the model in subsequent operations. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
In Ascend EP mode, allocate memory on the Host for the model parameter.
<!-- end id1 -->

<!-- npu="310b" id2 -->
In Ascend RC mode, allocate memory on the Device for the model parameter.
<!-- end id2 -->

<!-- npu="310p" id3 -->
In Control CPU open mode, allocate memory on the Device for the model parameter.
<!-- end id3 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlBundleInitFromMem_res.md#id2 -->
