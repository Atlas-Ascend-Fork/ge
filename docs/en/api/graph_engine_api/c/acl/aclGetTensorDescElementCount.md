# aclGetTensorDescElementCount

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:49:17.164Z pushedAt=2026-11-18T03:11:01.068Z nl=2 -->

## Product Support

<!-- npu="950" id1000 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1000 -->
<!-- npu="A3" id1001 -->
- Atlas A3 products: Supported
<!-- end id1001 -->
<!-- npu="910b" id1002 -->
- Atlas A2 products: Supported
<!-- end id1002 -->
<!-- npu="310b" id1003 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1003 -->
<!-- npu="310p" id1004 -->
- Atlas inference products: Supported
<!-- end id1004 -->
<!-- npu="910" id1005 -->
- Atlas training products: Supported
<!-- end id1005 -->
<!-- npu="IPV350" id1006 -->
- IPV350: not supported
<!-- end id1006 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescElementCount_res.md#id1 -->

## Function Description

Obtains the number of elements in a tensor description.

## Prototype

```c
size_t aclGetTensorDescElementCount(const aclTensorDesc *desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |

## Return Value

Returns the number of elements in the specified tensor description.
