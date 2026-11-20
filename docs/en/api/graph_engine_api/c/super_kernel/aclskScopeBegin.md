# aclskScopeBegin

<!-- md-trans-meta sourceCommit=c0c24c5cd1c86bd859c92aa19eb866b3cff12abf translatedAt=2026-11-17T15:54:30.107Z pushedAt=2026-11-18T07:01:19.600Z nl=2 -->

## Product Support

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Not supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Not supported
<!-- end id6 -->
<!-- npu="IPV350" id7 -->
- IPV350: not supported
<!-- end id7 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/super_kernel/aclskScopeBegin_res.md#id1 -->

## Header File

\#include <super\_kernel/super\_kernel.h\>

## Function Description

Marks the beginning position of a Super Kernel scope. After this API is called, a scope\_begin kernel and a placeholder kernel are issued on the specified Stream to mark the start boundary of a group of operator operations that need to be fused for execution.

## Prototype

```c
aclError aclskScopeBegin(const char* scopeName, aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| scopeName | Input | Name of the Super Kernel scope, used to uniquely identify a fusion scope.<br>If nullptr is passed, no fusion is performed. |
| stream | Input | Specified ACL runtime stream on which the scope marker kernel is executed. |

## Return Value

Returns ACL_SUCCESS on success; otherwise, returns a failure value.

## Constraints

- Use this API together with [aclskScopeEnd](aclskScopeEnd.md), and pass the same scopeName as that passed to aclskScopeEnd.
- The scope name must not exceed 256 bytes. Note that the scopeName input parameter is of the char type, and the actual valid name length must not exceed 255 bytes.
- The number of globally supported scope names (distinct) must not exceed 256.
