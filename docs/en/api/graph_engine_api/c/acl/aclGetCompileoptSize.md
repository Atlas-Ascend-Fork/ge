# aclGetCompileoptSize

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:39:43.272Z pushedAt=2026-11-18T03:10:28.067Z nl=2 -->

## Product Support

<!-- npu="950" id804 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id804 -->
<!-- npu="A3" id805 -->
- Atlas A3 products: Supported
<!-- end id805 -->
<!-- npu="910b" id806 -->
- Atlas A2 products: Supported
<!-- end id806 -->
<!-- npu="310b" id807 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id807 -->
<!-- npu="310p" id808 -->
- Atlas inference products: Supported
<!-- end id808 -->
<!-- npu="910" id809 -->
- Atlas training products: Supported
<!-- end id809 -->
<!-- npu="IPV350" id810 -->
- IPV350: not supported
<!-- end id810 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetCompileoptSize_res.md#id1 -->

## Function Description

You can call this API to obtain the memory size for storing the compilation option value.

Before calling [aclGetCompileopt](aclGetCompileopt.md), you need to allocate memory to store the compilation option value. The memory size can be obtained by calling this API.

## Prototype

```c
size_t aclGetCompileoptSize(aclCompileOpt opt)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opt | Input | Compile option. See [aclCompileOpt](aclCompileOpt.md) for the type definition. |

## Return Value

Returns the memory size required to store the value of the compilation option, in bytes.
