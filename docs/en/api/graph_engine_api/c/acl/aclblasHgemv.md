# aclblasHgemv

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:30:10.960Z pushedAt=2026-11-18T02:11:17.275Z nl=2 -->

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
<!-- npu="IPV350" id511 -->
- IPV350: not supported
<!-- end id511 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasHgemv_res.md#id1 -->

## Function Description

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

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transA | Input | Whether matrix A is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| m | Input | Number of rows of matrix A. Row-major order is used when storing matrix multiplication data. |
| n | Input | Number of columns of matrix A. |
| alpha | Input | Pointer to the scalar α used for the multiplication operation. See aclFloat16 for type definition. |
| a | Input | Pointer to matrix A. See aclFloat16 for type definition. |
| lda | Input | Leading dimension of matrix A. When transposition is selected and row-major order is used, lda is the number of columns of A. Reserved Parameter, which can only be set to -1. |
| x | Input | Pointer to vector x. See aclFloat16 for type definition. |
| incx | Input | Stride between consecutive elements of x.<br>Reserved Parameter, which can only be set to -1. |
| beta | Input | Pointer to the scalar β used for the multiplication operation. See aclFloat16 for type definition. |
| y | Input&Output | Pointer to vector y. See aclFloat16 for type definition. |
| incy | Input | Stride between consecutive elements of y.<br>Reserved Parameter, which can only be set to -1. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for the type definition. |
| stream | Input | Stream on which the operator runs. See aclrtStream for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
