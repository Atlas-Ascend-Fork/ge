# aclSetCompileopt

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:16:39.225Z pushedAt=2026-11-18T06:14:57.451Z nl=2 -->

## Product Support

<!-- npu="950" id393 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id393 -->
<!-- npu="A3" id394 -->
- Atlas A3 products: Supported
<!-- end id394 -->
<!-- npu="910b" id395 -->
- Atlas A2 products: Supported
<!-- end id395 -->
<!-- npu="310b" id396 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id396 -->
<!-- npu="310p" id397 -->
- Atlas inference products: Supported
<!-- end id397 -->
<!-- npu="910" id398 -->
- Atlas training products: Supported
<!-- end id398 -->
<!-- npu="IPV350" id399 -->
- IPV350: not supported
<!-- end id399 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetCompileopt_res.md#id1 -->

## Function Description

You can call this API to set the corresponding compile option, which is shared globally at the process level. Call this API to set the compile option before operator or model compilation. During compilation, the current compile option takes effect and remains unchanged throughout a single compilation process.

## Prototype

```c
aclError aclSetCompileopt(aclCompileOpt opt, const char *value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opt | Input | Compile option. See [aclCompileOpt](aclCompileOpt.md) for the type definition. |
| value | Input | Pointer to the specific value of the compile option. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
