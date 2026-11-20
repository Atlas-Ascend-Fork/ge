# aclopSetKernelArgs

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:12:39.465Z pushedAt=2026-11-18T06:14:08.632Z nl=2 -->

## Product Support

<!-- npu="950" id1105 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id1105 -->
<!-- npu="A3" id1106 -->
- Atlas A3 products: Supported
<!-- end id1106 -->
<!-- npu="910b" id1107 -->
- Atlas A2 products: Supported
<!-- end id1107 -->
<!-- npu="310b" id1108 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1108 -->
<!-- npu="310p" id1109 -->
- Atlas inference products: Supported
<!-- end id1109 -->
<!-- npu="910" id1110 -->
- Atlas training products: Supported
<!-- end id1110 -->
<!-- npu="IPV350" id1111 -->
- IPV350: not supported
<!-- end id1111 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetKernelArgs_res.md#id1 -->

## Function Description

In dynamic shape scenarios, sets the operator Tiling parameters and the number of concurrent executions.

## Prototype

```c
aclError aclopSetKernelArgs(aclopKernelDesc *kernelDesc,
const char *kernelId,
uint32_t blockDim,
const void *args,
uint32_t argSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| kernelDesc | Input | Kernel descriptor cache, a pointer of the aclopKernelDesc type.<br>typedef struct aclopKernelDesc aclopKernelDesc; |
| kernelId | Input | Pointer to the Kernel ID to be specified during operator execution, which must be the same as the kernelId passed when calling [aclopCreateKernel](aclopCreateKernel.md). |
| blockDim | Input | Number of concurrent AI Core cores for kernel execution.<br>It is recommended that blockDim be consistent with the number of AI Core cores used in the TIK operator implementation. |
| args | Input | Pointer to the Tiling parameter. |
| argSize | Input | Memory size of the Tiling parameter, in bytes. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
