# aclgrphProfFinalize

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:10:14.241Z pushedAt=2026-10-28T07:40:22.125Z nl=2 -->

## Applicable Products

<!-- npu="950" id1 -->
- Ascend 950PR&950DT: Supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2: Supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Supported
<!-- end id6 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphProfFinalize_res.md#id1 -->

## Header File/Library File

- Header file: \#include <ge/ge\_prof.h\>
- Library file: libmsprofiler.so

## Description

Ends Profiling.

## Prototype

```c++
Status aclgrphProfFinalize()
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | SUCCESS: Success<br>Others: Failure |

## Constraints

This API must be used in pairs with [aclgrphProfInit](aclgrphProfInit.md). Call **aclgrphProfInit** first and then **aclgrphProfFinalize**, and it only needs to be called once.
