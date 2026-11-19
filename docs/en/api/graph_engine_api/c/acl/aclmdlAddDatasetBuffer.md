# aclmdlAddDatasetBuffer

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:52:38.114Z pushedAt=2026-11-18T03:15:42.934Z nl=2 -->

## Product Support

<!-- npu="950" id183 -->
- Ascend 950PR&950DT products: Supported
<!-- end id183 -->
<!-- npu="A3" id184 -->
- Atlas A3 products: Supported
<!-- end id184 -->
<!-- npu="910b" id185 -->
- Atlas A2 products: Supported
<!-- end id185 -->
<!-- npu="310b" id186 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id186 -->
<!-- npu="310p" id187 -->
- Atlas inference products: Supported
<!-- end id187 -->
<!-- npu="910" id188 -->
- Atlas training products: Supported
<!-- end id188 -->
<!-- npu="IPV350" id189 -->
- IPV350: support
<!-- end id189 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlAddDatasetBuffer_res.md#id1 -->

## Function Description

Adds an aclDataBuffer to aclmdlDataset.

## Prototype

```c
aclError aclmdlAddDatasetBuffer(aclmdlDataset *dataset, aclDataBuffer *dataBuffer)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dataset | Output | Pointer to the aclmdlDataset to which the aclDataBuffer is to be added.<br>Call [aclmdlCreateDataset](aclmdlCreateDataset.md) in advance to create the aclmdlDataset data. |
| dataBuffer | Input | Pointer to the aclDataBuffer to be added.<br>Call the aclCreateDataBuffer API in advance to create the aclDataBuffer data. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
