# aclblasGemvEx

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:32:25.584Z pushedAt=2026-10-29T02:21:40.427Z nl=2 -->

## Product Support

<!-- npu="950" id414 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id414 -->
<!-- npu="A3" id415 -->
- Atlas A3 products: Supported
<!-- end id415 -->
<!-- npu="910b" id416 -->
- Atlas A2 products: Supported
<!-- end id416 -->
<!-- npu="310b" id417 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id417 -->
<!-- npu="310p" id418 -->
- Atlas inference products: Supported
<!-- end id418 -->
<!-- npu="910" id419 -->
- Atlas training products: Supported
<!-- end id419 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasGemvEx_res.md#id1 -->

## Feature Description

Performs matrix-vector multiplication, y = αAx + βy. The data types of the input and output data are set through input parameters. This is an asynchronous API.

The data types of A, x, and y support only the following combinations. The data types of α and β are the same as that of y.

| Data Type of A | Data Type of x | Data Type of y |
| --- | --- | --- |
| aclFloat16 | aclFloat16 | aclFloat16 |
| aclFloat16 | aclFloat16 | float(float32) |
| int8_t | int8_t | float(float32) |
| int8_t | int8_t | int32_t |

## Prototype

```c
aclError aclblasGemvEx(aclTransType transA,
int m,
int n,
const void *alpha,
const void *a,
int lda,
aclDataType dataTypeA,
const void *x,
int incx,
aclDataType dataTypeX,
const void *beta,
void *y,
int incy,
aclDataType dataTypeY,
aclComputeType type,
aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transA | Input | Flag indicating whether matrix A is transposed. See [aclTransType](aclTransType.md) for type definition. |
| m | Input | Number of rows of matrix A. Row-major order is used when storing matrix multiplication data. |
| n | Input | Number of columns of matrix A. |
| alpha | Input | Pointer to the scalar α used for the multiplication operation. |
| a | Input | Pointer to matrix A. |
| lda | Input | Leading dimension of matrix A. When transposition is selected and row-major order is used, lda is the number of columns of A. Reserved parameter, which can only be set to -1. |
| dataTypeA | Input | Data type of matrix A. See aclDataType for type definition. |
| x | Input | Pointer to vector x. |
| incx | Input | Stride between consecutive elements of x.<br>Reserved parameter, which can only be set to -1. |
| dataTypeX | Input | Data type of vector x. See aclDataType for type definition. |
| beta | Input | Pointer to the scalar β used for the multiplication operation. |
| y | Input&Output | Pointer to vector y. |
| incy | Input | Stride between consecutive elements of y.<br>Reserved parameter, which can only be set to -1. |
| dataTypeY | Input | Data type of vector y. See aclDataType for type definition. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for type definition. |
| stream | Input | Stream on which the operator is executed. See aclrtStream for type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
