# aclopSetKernelArgs

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:56:39.608Z pushedAt=2026-10-29T03:58:00.984Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetKernelArgs_res.md#id1 -->

## Feature Description

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
