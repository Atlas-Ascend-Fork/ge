# aclopUnregisterCompileFunc

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:14:40.718Z pushedAt=2026-11-18T06:14:39.781Z nl=2 -->

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
<!-- npu="IPV350" id831 -->
- IPV350: not supported
<!-- end id831 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopUnregisterCompileFunc_res.md#id1 -->

## Function Description

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
