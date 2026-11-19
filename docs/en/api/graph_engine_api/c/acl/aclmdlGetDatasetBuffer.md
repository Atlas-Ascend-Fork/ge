# aclmdlGetDatasetBuffer

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:17:15.139Z pushedAt=2026-11-18T03:30:05.193Z nl=2 -->

## Product Support

<!-- npu="950" id1189 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1189 -->
<!-- npu="A3" id1190 -->
- Atlas A3 products: Supported
<!-- end id1190 -->
<!-- npu="910b" id1191 -->
- Atlas A2 products: Supported
<!-- end id1191 -->
<!-- npu="310b" id1192 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1192 -->
<!-- npu="310p" id1193 -->
- Atlas inference products: Supported
<!-- end id1193 -->
<!-- npu="910" id1194 -->
- Atlas training products: Supported
<!-- end id1194 -->
<!-- npu="IPV350" id1195 -->
- IPV350: support
<!-- end id1195 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDatasetBuffer_res.md#id1 -->

## Function Description

Obtains the nth aclDataBuffer in aclmdlDataset.

## Prototype

```c
aclDataBuffer* aclmdlGetDatasetBuffer(const aclmdlDataset *dataset, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dataset | Input | Pointer to the aclmdlDataset type.<br>Call [aclmdlCreateDataset](aclmdlCreateDataset.md) in advance to create data of the aclmdlDataset type. |
| index | Input | Index of the aclDataBuffer to obtain. |

## Return Value

- Returns the address of aclDataBuffer on success.
- Returns a null address on failure.
