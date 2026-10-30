# aclopUnregisterCompileFunc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:58:23.254Z pushedAt=2026-10-29T03:58:37.185Z nl=2 -->

## Product Support

<!-- npu="950" id825 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id825 -->
<!-- npu="A3" id826 -->
- Atlas A3 products: Supported
<!-- end id826 -->
<!-- npu="910b" id827 -->
- Atlas A2 products: Supported
<!-- end id827 -->
<!-- npu="310b" id828 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id828 -->
<!-- npu="310p" id829 -->
- Atlas inference products: Supported
<!-- end id829 -->
<!-- npu="910" id830 -->
- Atlas training products: Supported
<!-- end id830 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopUnregisterCompileFunc_res.md#id1 -->

## Feature Description

Unregisters the operator selector in dynamic shape scenarios.

## Prototype

```c
aclError aclopUnregisterCompileFunc(const char *opType)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
