# aclblasS8gemm

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:33:41.880Z pushedAt=2026-10-29T02:22:08.480Z nl=2 -->

## Product Support

<!-- npu="950" id204 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id204 -->
<!-- npu="A3" id205 -->
- Atlas A3 products: Supported
<!-- end id205 -->
<!-- npu="910b" id206 -->
- Atlas A2 products: Supported
<!-- end id206 -->
<!-- npu="310b" id207 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id207 -->
<!-- npu="310p" id208 -->
- Atlas inference products: Supported
<!-- end id208 -->
<!-- npu="910" id209 -->
- Atlas training products: Supported
<!-- end id209 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasS8gemm_res.md#id1 -->

## Feature Description

Performs matrix-matrix multiplication, C = αAB + βC, where the input data type is int8\_t and the output data type is int32\_t. This is an asynchronous API.

## Prototype

```c
aclError aclblasS8gemm(aclTransType transA,
aclTransType transB,
aclTransType transC,
int m,
int n,
int k,
const int32_t *alpha,
const int8_t *matrixA,
int lda,
const int8_t *matrixB,
int ldb,
const int32_t *beta,
int32_t *matrixC,
int ldc,
aclComputeType type,
aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transA | Input | Flag indicating whether matrix A is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| transB | Input | Flag indicating whether matrix B is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| transC | Input | Flag for matrix C. See [aclTransType](aclTransType.md) for the type definition.<br>Currently, only ACL_TRANS_N is supported. |
| m | Input | Number of rows of matrix A and matrix C. |
| n | Input | Number of columns of matrix B and matrix C. |
| k | Input | Number of columns of matrix A and number of rows of matrix B. |
| alpha | Input | Pointer to the scalar α used for the multiplication operation. |
| matrixA | Input | Pointer to matrix A. |
| lda | Input | Matrix leading dimension of A. When transposition is selected and row-major order is used, lda is the number of columns of A. Reserved parameter, which can only be set to -1. |
| matrixB | Input | Pointer to matrix B. |
| ldb | Input | Matrix leading dimension of B. When transposition is selected and row-major order is used, ldb is the number of columns of B. Reserved parameter, which can only be set to -1. |
| beta | Input | Pointer to the scalar β used for the multiplication operation. |
| matrixC | Input&Output | Pointer to matrix C. |
| ldc | Input | Matrix leading dimension of C. Reserved parameter, which can only be set to -1. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for the type definition. |
| stream | Input | Stream on which the operator is executed. See aclrtStream for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
