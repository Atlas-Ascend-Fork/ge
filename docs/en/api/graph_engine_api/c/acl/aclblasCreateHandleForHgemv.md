# aclblasCreateHandleForHgemv

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:30:33.558Z pushedAt=2026-10-29T02:10:58.752Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasCreateHandleForHgemv_res.md#id1 -->

## Feature Description

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
| transA | Input | Whether matrix A is transposed. For the type definition, see also [aclTransType](aclTransType.md). |
| m | Input | Number of rows of matrix A, row-major when storing matrix multiplication data. |
| n | Input | Number of columns of matrix A. |
| type | Input | Computation precision, defaulting to high precision. For the type definition, see also [aclComputeType](aclComputeType.md). |
| handle | Output | Pointer to the pointer to the operator execution handle. For the type definition, see also [aclopHandle](aclopHandle.md). |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See also [aclError](aclError.md).
