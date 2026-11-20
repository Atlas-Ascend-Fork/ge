# aclmdlGetOutputIndexByName

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:30:43.635Z pushedAt=2026-11-18T03:33:03.186Z nl=2 -->

## Product Support

<!-- npu="950" id581 -->
- Ascend 950PR&950DT products: Supported
<!-- end id581 -->
<!-- npu="A3" id582 -->
- Atlas A3 products: Supported
<!-- end id582 -->
<!-- npu="910b" id583 -->
- Atlas A2 products: Supported
<!-- end id583 -->
<!-- npu="310b" id584 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id584 -->
<!-- npu="310p" id585 -->
- Atlas inference products: Supported
<!-- end id585 -->
<!-- npu="910" id586 -->
- Atlas training products: Supported
<!-- end id586 -->
<!-- npu="IPV350" id587 -->
- IPV350: support
<!-- end id587 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetOutputIndexByName_res.md#id1 -->

## Function Description

Obtains the output index corresponding to an output name in an om/om2 model.

## Prototype

```c
aclError aclmdlGetOutputIndexByName(const aclmdlDesc *modelDesc, const char *name, size_t *index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| name | Input | Pointer to the output name. |
| index | Output | Pointer to the output index. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
