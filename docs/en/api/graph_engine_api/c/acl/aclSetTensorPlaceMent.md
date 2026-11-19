# aclSetTensorPlaceMent

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:20:11.817Z pushedAt=2026-11-18T06:26:58.256Z nl=2 -->

## Product Support

<!-- npu="950" id435 -->
- Ascend 950PR&950DT products: Supported
<!-- end id435 -->
<!-- npu="A3" id436 -->
- Atlas A3 products: Supported
<!-- end id436 -->
<!-- npu="910b" id437 -->
- Atlas A2 products: Supported
<!-- end id437 -->
<!-- npu="310b" id438 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id438 -->
<!-- npu="310p" id439 -->
- Atlas inference products: Supported
<!-- end id439 -->
<!-- npu="910" id440 -->
- Atlas training products: Supported
<!-- end id440 -->
<!-- npu="IPV350" id441 -->
- IPV350: not supported
<!-- end id441 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorPlaceMent_res.md#id1 -->

## Function Description

Sets the placement attribute of TensorDesc to identify the memory type for storing tensor data.

## Prototype

```c
aclError aclSetTensorPlaceMent(aclTensorDesc *desc, aclMemType memType)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| memType | Input | Specifies the placement attribute value. See aclMemType for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
