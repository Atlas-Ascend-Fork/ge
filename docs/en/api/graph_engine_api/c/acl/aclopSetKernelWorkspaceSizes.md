# aclopSetKernelWorkspaceSizes

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:57:03.229Z pushedAt=2026-10-29T03:58:06.105Z nl=2 -->

## Product Support

<!-- npu="950" id944 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id944 -->
<!-- npu="A3" id945 -->
- Atlas A3 products: Supported
<!-- end id945 -->
<!-- npu="910b" id946 -->
- Atlas A2 products: Supported
<!-- end id946 -->
<!-- npu="310b" id947 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id947 -->
<!-- npu="310p" id948 -->
- Atlas inference products: Supported
<!-- end id948 -->
<!-- npu="910" id949 -->
- Atlas training products: Supported
<!-- end id949 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetKernelWorkspaceSizes_res.md#id1 -->

## Feature Description

In dynamic shape scenarios, sets the operator Workspace parameters. This is an optional API, which can be used depending on the operator.

## Prototype

```c
aclError aclopSetKernelWorkspaceSizes(aclopKernelDesc *kernelDesc, int numWorkspaces, size_t *workspaceSizes)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| kernelDesc | Input | Kernel descriptor cache, a pointer of the aclopKernelDesc type.<br>typedef struct aclopKernelDesc aclopKernelDesc; |
| numWorkspaces | Input | Number of workspaces. |
| workspaceSizes | Input | Pointer to the array of workspace sizes. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
