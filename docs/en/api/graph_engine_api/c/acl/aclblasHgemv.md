# aclblasHgemv

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:33:14.854Z pushedAt=2026-10-29T02:21:58.414Z nl=2 -->

## Product Support

<!-- npu="950" id505 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id505 -->
<!-- npu="A3" id506 -->
- Atlas A3 products: Supported
<!-- end id506 -->
<!-- npu="910b" id507 -->
- Atlas A2 products: Supported
<!-- end id507 -->
<!-- npu="310b" id508 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id508 -->
<!-- npu="310p" id509 -->
- Atlas inference products: Supported
<!-- end id509 -->
<!-- npu="910" id510 -->
- Atlas training products: Supported
<!-- end id510 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasHgemv_res.md#id1 -->

## Feature Description

Performs matrix-vector multiplication, y = αAx + βy, where the input and output data type is aclFloat16**.** Asynchronous API.

## Prototype

```c
aclError aclblasHgemv(aclTransType transA,
int m,
int n,
const aclFloat16 *alpha,
const aclFloat16 *a,
int lda,
const aclFloat16 *x,
int incx,
const aclFloat16 *beta,
aclFloat16 *y,
int incy,
aclComputeType type,
aclrtStream stream)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| transA | Input | Whether matrix A is transposed. See [aclTransType](aclTransType.md) for type definition. |
| m | Input | Number of rows of matrix A. Row-major order is used when storing matrix multiplication data. |
| n | Input | Number of columns of matrix A. |
| alpha | Input | Pointer to the scalar α used for the multiplication operation. See aclFloat16 for type definition. |
| a | Input | Pointer to matrix A. See aclFloat16 for type definition. |
| lda | Input | Leading dimension of matrix A. When transposition is selected and row-major order is used, lda is the number of columns of A. Reserved parameter. Currently, it can only be set to -1. |
| x | Input | Pointer to vector x. See aclFloat16 for type definition. |
| incx | Input | Stride between consecutive elements of x.<br>Reserved parameter. Currently, it can only be set to -1. |
| beta | Input | Pointer to the scalar β used for the multiplication operation. See aclFloat16 for type definition. |
| y | Input&Output | Pointer to vector y. See aclFloat16 for type definition. |
| incy | Input | Stride between consecutive elements of y.<br>Reserved parameter. Currently, it can only be set to -1. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for type definition. |
| stream | Input | Stream on which the operator is executed. See aclrtStream for type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
