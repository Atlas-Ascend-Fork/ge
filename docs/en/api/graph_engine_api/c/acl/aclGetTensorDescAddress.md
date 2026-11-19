# aclGetTensorDescAddress

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:45:46.312Z pushedAt=2026-11-18T03:10:32.270Z nl=2 -->

## Product Support

<!-- npu="950" id1126 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1126 -->
<!-- npu="A3" id1127 -->
- Atlas A3 products: Supported
<!-- end id1127 -->
<!-- npu="910b" id1128 -->
- Atlas A2 products: Supported
<!-- end id1128 -->
<!-- npu="310b" id1129 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1129 -->
<!-- npu="310p" id1130 -->
- Atlas inference products: Supported
<!-- end id1130 -->
<!-- npu="910" id1131 -->
- Atlas training products: Supported
<!-- end id1131 -->
<!-- npu="IPV350" id1132 -->
- IPV350: not supported
<!-- end id1132 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescAddress_res.md#id1 -->

## Function Description

Obtains the memory address of the tensor data for the specified operator input/output.

## Prototype

```c
void *aclGetTensorDescAddress(const aclTensorDesc *desc)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call [aclGetTensorDescByIndex](aclGetTensorDescByIndex.md) to obtain the tensor description of the specified input/output of the operator, and use it as the input of this API. |

## Return Value

Returns the memory address of the tensor data of the specified operator input/output.
