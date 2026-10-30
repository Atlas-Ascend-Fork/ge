# aclmdlCreateAIPP

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:59:14.943Z pushedAt=2026-10-29T02:59:31.795Z nl=2 -->

## Product Support

<!-- npu="950" id29 -->
- Ascend 950PR&950DT products: Supported
<!-- end id29 -->
<!-- npu="A3" id30 -->
- Atlas A3 products: Supported
<!-- end id30 -->
<!-- npu="910b" id31 -->
- Atlas A2 products: Supported
<!-- end id31 -->
<!-- npu="310b" id32 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id32 -->
<!-- npu="310p" id33 -->
- Atlas inference products: Supported
<!-- end id33 -->
<!-- npu="910" id34 -->
- Atlas training products: Supported
<!-- end id34 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlCreateAIPP_res.md#id1 -->

## Feature Description

In dynamic AIPP scenarios, creates data of the aclmdlAIPP type based on the batch size supported by the model to store dynamic AIPP parameters.

To destroy data of the aclmdlAIPP type, see [aclmdlDestroyAIPP](aclmdlDestroyAIPP.md).

## Prototype

```c
aclmdlAIPP *aclmdlCreateAIPP(uint64_t batchSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| batchSize | Input | Batch size of the original model. |

## Return Value

Returns a pointer of the aclmdlAIPP type.
