# aclblasS8gemv

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:31:46.883Z pushedAt=2026-11-18T02:08:41.518Z nl=2 -->

## Product Support

<!-- npu="950" id1077 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id1077 -->
<!-- npu="A3" id1078 -->
- Atlas A3 products: Supported
<!-- end id1078 -->
<!-- npu="910b" id1079 -->
- Atlas A2 products: Supported
<!-- end id1079 -->
<!-- npu="310b" id1080 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1080 -->
<!-- npu="310p" id1081 -->
- Atlas inference products: Supported
<!-- end id1081 -->
<!-- npu="910" id1082 -->
- Atlas training products: Supported
<!-- end id1082 -->
<!-- npu="IPV350" id1083 -->
- IPV350: not supported
<!-- end id1083 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasS8gemv_res.md#id1 -->

## Function Description

Performs matrix-vector multiplication, y = αAx + βy, with input data of type int8\_t and output data of type int32\_t. This is an asynchronous API.

## Prototype

```c
aclError aclblasS8gemv(aclTransType transA,
int m,
int n,
const int32_t *alpha,
const int8_t *a,
int lda,
const int8_t *x,
int incx,
const int32_t *beta,
int32_t *y,
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
| alpha | Input | Pointer to the scalar α used for the multiplication operation. |
| a | Input | Pointer to matrix A. |
| lda | Input | Leading dimension of matrix A. When transposition is selected and row-major order is used, lda is the number of columns of A. Reserved Parameter, which can only be set to -1. |
| x | Input | Pointer to vector x. |
| incx | Input | Stride between consecutive elements of x.<br>Reserved Parameter, which can only be set to -1. |
| beta | Input | Pointer to the scalar β used for the multiplication operation. |
| y | Input&Output | Pointer to vector y. |
| incy | Input | Stride between consecutive elements of y.<br>Reserved Parameter, which can only be set to -1. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for the type definition. |
| stream | Input | Stream on which the operator runs. See aclrtStream for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
