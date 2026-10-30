# aclskScopeEnd

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:37:06.108Z pushedAt=2026-10-28T13:43:03.637Z nl=2 -->

## Product Support

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Not supported
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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/super_kernel/aclskScopeEnd_res.md#id1 -->

## Header File

\#include <super\_kernel/super\_kernel.h\>

## Feature Description

Marks the end position of the Super Kernel scope. After this API is called, a scope\_begin kernel and a placeholder kernel are delivered on the specified stream to mark the end boundary of a group of operator operations that need to be fused for execution.

## Prototype

```c
aclError aclskScopeEnd(const char* scopeName, aclrtStream stream)
```

## Parameters

| parameter name | Input/Output | Description |
| --- | --- | --- |
| scopeName | Input | Name of the Super Kernel scope, used to uniquely identify a fusion scope.<br>If nullptr is passed, no fusion is performed. |
| stream | Input | Specified ACL runtime stream on which the scope marker kernel is executed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value.

Returns ACL\_ERROR\_INVALID\_PARAM to indicate that the parameter is invalid (scopeName is an empty string array, or its length exceeds 255 characters).

## Constraints

- Used in pairs with [aclskScopeBegin](aclskScopeBegin.md). The scopeName passed in must be the same as that in aclskScopeBegin.
- The scope name must not exceed 256 bytes. Note: The scopeName input parameter is of the char type, and the actual valid name length of scopeName must not exceed 255 bytes.
- The number of globally supported scope names (distinct) must not exceed 256.
