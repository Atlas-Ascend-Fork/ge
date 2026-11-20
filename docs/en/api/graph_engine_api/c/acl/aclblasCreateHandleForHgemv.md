# aclblasCreateHandleForHgemv

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:27:10.569Z pushedAt=2026-11-17T13:44:37.491Z nl=2 -->

## Product Support

<!-- npu="950" id560 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id560 -->
<!-- npu="A3" id561 -->
- Atlas A3 products: Supported
<!-- end id561 -->
<!-- npu="910b" id562 -->
- Atlas A2 products: Supported
<!-- end id562 -->
<!-- npu="310b" id563 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id563 -->
<!-- npu="310p" id564 -->
- Atlas inference products: Supported
<!-- end id564 -->
<!-- npu="910" id565 -->
- Atlas training products: Supported
<!-- end id565 -->
<!-- npu="IPV350" id566 -->
- IPV350: not supported
<!-- end id566 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasCreateHandleForHgemv_res.md#id1 -->

## Function Description

Creates a handle for matrix-vector multiplication, where the input and output data types are aclFloat16. After the handle is created, call the [aclopExecWithHandle](aclopExecWithHandle.md) API to execute the operator.

## Prototype

```c
aclError aclblasCreateHandleForHgemv(aclTransType transA,
int m,
int n,
aclComputeType type,
aclopHandle **handle)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transA | Input | Whether matrix A is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| m | Input | Number of rows of matrix A. Row-major order is used when storing matrix multiplication data. |
| n | Input | Number of columns of matrix A. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for the type definition. |
| handle | Output | Pointer to a pointer to the operator execution handle. See [aclopHandle](aclopHandle.md) for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
