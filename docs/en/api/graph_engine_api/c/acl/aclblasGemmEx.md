# aclblasGemmEx

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:31:51.373Z pushedAt=2026-10-29T02:21:29.749Z nl=2 -->

## Product Support

<!-- npu="950" id965 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id965 -->
<!-- npu="A3" id966 -->
- Atlas A3 products: Supported
<!-- end id966 -->
<!-- npu="910b" id967 -->
- Atlas A2 products: Supported
<!-- end id967 -->
<!-- npu="310b" id968 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id968 -->
<!-- npu="310p" id969 -->
- Atlas inference products: Supported
<!-- end id969 -->
<!-- npu="910" id970 -->
- Atlas training products: Supported
<!-- end id970 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasGemmEx_res.md#id1 -->

## Feature Description

Performs matrix-matrix multiplication, C = αAB + βC. The data types of the input and output data are set through input parameters. This is an asynchronous API.

The data types of A, B, and C support only the following combinations. The data types of α and β are the same as that of C.

| Data Type of A | Data Type of B | Data Type of C |
| --- | --- | --- |
| aclFloat16 | aclFloat16 | aclFloat16 |
| aclFloat16 | aclFloat16 | float(float32) |
| int8_t | int8_t | float(float32) |
| int8_t | int8_t | int32_t |

## Prototype

```c
aclError aclblasGemmEx(aclTransType transA,
aclTransType transB,
aclTransType transC,
int m,
int n,
int k,
const void *alpha,
const void *matrixA,
int lda,
aclDataType dataTypeA,
const void *matrixB,
int ldb,
aclDataType dataTypeB,
const void *beta,
void *matrixC,
int ldc,
aclDataType dataTypeC,
aclComputeType type,
aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transA | Input | Whether matrix A is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| transB | Input | Whether matrix B is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| transC | Input | Flag of matrix C. See [aclTransType](aclTransType.md) for the type definition.<br>Currently, only ACL_TRANS_N is supported. |
| m | Input | Number of rows of matrix A and matrix C. |
| n | Input | Number of columns of matrix B and matrix C. |
| k | Input | Number of columns of matrix A and number of rows of matrix B. |
| alpha | Input | Pointer to the scalar α used for the multiplication operation. |
| matrixA | Input | Pointer to matrix A. |
| lda | Input | Matrix leading dimension of A. When transposition is selected and row-major order is used, lda is the number of columns of A. Reserved parameter, which can only be set to -1. |
| dataTypeA | Input | Data type of matrix A. See aclDataType for the type definition. |
| matrixB | Input | Pointer to matrix B. |
| ldb | Input | Matrix leading dimension of B. When transposition is selected and row-major order is used, ldb is the number of columns of B. Reserved parameter, which can only be set to -1. |
| dataTypeB | Input | Data type of matrix B. See aclDataType for the type definition. |
| beta | Input | Pointer to the scalar β used for the multiplication operation. |
| matrixC | Input&Output | Pointer to matrix C. |
| ldc | Input | Matrix leading dimension of C. Reserved parameter, which can only be set to -1. |
| dataTypeC | Input | Data type of matrix C. See aclDataType for the type definition. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for the type definition. |
| stream | Input | Stream on which the operator runs. See aclrtStream for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
