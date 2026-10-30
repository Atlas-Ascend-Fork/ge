# aclblasCreateHandleForS8gemv

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:31:30.603Z pushedAt=2026-10-29T02:20:34.262Z nl=2 -->

## Product Support

<!-- npu="950" id783 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id783 -->
<!-- npu="A3" id784 -->
- Atlas A3 products: Supported
<!-- end id784 -->
<!-- npu="910b" id785 -->
- Atlas A2 products: Supported
<!-- end id785 -->
<!-- npu="310b" id786 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id786 -->
<!-- npu="310p" id787 -->
- Atlas inference products: Supported
<!-- end id787 -->
<!-- npu="910" id788 -->
- Atlas training products: Supported
<!-- end id788 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasCreateHandleForS8gemv_res.md#id1 -->

## Feature Description

Creates a handle for matrix-vector multiplication, where the input data type is int8\_t and the output data type is int32\_t. After the handle is created, call the [aclopExecWithHandle](aclopExecWithHandle.md) API to execute the operator.

## Prototype

```c
aclError aclblasCreateHandleForS8gemv(aclTransType transA,
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
| handle | Output | Pointer to the "pointer to the operator execution handle". See [aclopHandle](aclopHandle.md) for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
