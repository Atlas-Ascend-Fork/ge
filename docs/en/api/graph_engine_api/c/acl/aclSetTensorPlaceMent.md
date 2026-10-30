# aclSetTensorPlaceMent

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:03:09.213Z pushedAt=2026-10-29T03:59:50.955Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorPlaceMent_res.md#id1 -->

## Feature Description

Sets the placement attribute of TensorDesc to identify the memory type for storing tensor data.

## Prototype

```c
aclError aclSetTensorPlaceMent(aclTensorDesc *desc, aclMemType memType)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type. |
| memType | Input | Specifies the placement attribute value. See aclMemType for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
