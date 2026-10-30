# aclblasHgemm

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:32:41.911Z pushedAt=2026-10-29T02:21:49.987Z nl=2 -->

## Product Support

<!-- npu="950" id1147 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id1147 -->
<!-- npu="A3" id1148 -->
- Atlas A3 products: Supported
<!-- end id1148 -->
<!-- npu="910b" id1149 -->
- Atlas A2 products: Supported
<!-- end id1149 -->
<!-- npu="310b" id1150 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1150 -->
<!-- npu="310p" id1151 -->
- Atlas inference products: Supported
<!-- end id1151 -->
<!-- npu="910" id1152 -->
- Atlas training products: Supported
<!-- end id1152 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasHgemm_res.md#id1 -->

## Feature Description

Performs matrix-matrix multiplication, C = αAB + βC, where the input and output data types are aclFloat16. This is an asynchronous API.

## Prototype

```c
aclError aclblasHgemm(aclTransType transA,
aclTransType transB,
aclTransType transC,
int m,
int n,
int k,
const aclFloat16 *alpha,
const aclFloat16 *matrixA,
int lda,
const aclFloat16 *matrixB,
int ldb,
const aclFloat16 *beta,
aclFloat16 *matrixC,
int ldc,
aclComputeType type,
aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transA | Input | Flag indicating whether matrix A is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| transB | Input | Flag indicating whether matrix B is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| transC | Input | Flag of matrix C. See [aclTransType](aclTransType.md) for the type definition.<br>Currently only ACL_TRANS_N is supported. |
| m | Input | Number of rows of matrix A and matrix C. |
| n | Input | Number of columns of matrix B and matrix C. |
| k | Input | Number of columns of matrix A and number of rows of matrix B. |
| alpha | Input | Pointer to the scalar α used for the multiplication operation. See aclFloat16 for the type definition. |
| matrixA | Input | Pointer to matrix A. See aclFloat16 for the type definition. |
| lda | Input | Leading dimension of matrix A. When transposition is selected and row-major order is used, lda is the number of columns of A. Reserved parameter, currently only set to -1. |
| matrixB | Input | Pointer to matrix B. See aclFloat16 for the type definition. |
| ldb | Input | Leading dimension of matrix B. When transposition is selected and row-major order is used, ldb is the number of columns of B.<br>Reserved parameter, currently only set to -1. |
| beta | Input | Pointer to the scalar β used for the multiplication operation. See aclFloat16 for the type definition. |
| matrixC | Input&Output | Pointer to matrix C. See aclFloat16 for the type definition. |
| ldc | Input | Leading dimension of matrix C. Reserved parameter, currently only set to -1. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for the type definition. |
| stream | Input | Stream on which the operator is executed. See aclrtStream for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
