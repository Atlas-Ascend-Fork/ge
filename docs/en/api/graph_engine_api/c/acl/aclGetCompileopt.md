# aclGetCompileopt

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:38:46.982Z pushedAt=2026-11-18T03:07:52.777Z nl=2 -->

## Product Support

<!-- npu="950" id706 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id706 -->
<!-- npu="A3" id707 -->
- Atlas A3 products: Supported
<!-- end id707 -->
<!-- npu="910b" id708 -->
- Atlas A2 products: Supported
<!-- end id708 -->
<!-- npu="310b" id709 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id709 -->
<!-- npu="310p" id710 -->
- Atlas inference products: Supported
<!-- end id710 -->
<!-- npu="910" id711 -->
- Atlas training products: Supported
<!-- end id711 -->
<!-- npu="IPV350" id712 -->
- IPV350: not supported
<!-- end id712 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetCompileopt_res.md#id1 -->

## Function Description

You can call this API to obtain the value of a compile option.

You can call this API to obtain the current value of a compile option. Before [aclSetCompileopt](aclSetCompileopt.md) is called, this API returns the default value of each compile option; after [aclSetCompileopt](aclSetCompileopt.md) is called, this API returns the value set by you.

## Prototype

```c
aclError aclGetCompileopt(aclCompileOpt opt, char *value, size_t length)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opt | Input | Compile option. See [aclCompileOpt](aclCompileOpt.md) for the type definition. |
| value | Output | Memory pointer that stores the compile option value.<br>Before calling this API, you need to allocate memory to store the compile option value. The memory size can be obtained by calling [aclGetCompileoptSize](aclGetCompileoptSize.md). |
| length | Input | Size of the memory pointed to by value, in bytes. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
