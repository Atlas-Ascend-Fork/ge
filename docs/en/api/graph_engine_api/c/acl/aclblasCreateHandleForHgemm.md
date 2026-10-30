# aclblasCreateHandleForHgemm

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:30:00.647Z pushedAt=2026-10-29T02:10:41.667Z nl=2 -->

## Product Support

<!-- npu="950" id484 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id484 -->
<!-- npu="A3" id485 -->
- Atlas A3 products: Supported
<!-- end id485 -->
<!-- npu="910b" id486 -->
- Atlas A2 products: Supported
<!-- end id486 -->
<!-- npu="310b" id487 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id487 -->
<!-- npu="310p" id488 -->
- Atlas inference products: Supported
<!-- end id488 -->
<!-- npu="910" id489 -->
- Atlas training products: Supported
<!-- end id489 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasCreateHandleForHgemm_res.md#id1 -->

## Feature Description

Creates a handle for matrix-matrix multiplication, where the input and output data type is aclFloat16. After the handle is created, call [aclopExecWithHandle](aclopExecWithHandle.md) to execute the operator.

## Prototype

```c
aclError aclblasCreateHandleForHgemm(aclTransType transA,
aclTransType transB,
aclTransType transC,
int m,
int n,
int k,
aclComputeType type,
aclopHandle **handle)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transA | Input | Flag indicating whether matrix A is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| transB | Input | Flag indicating whether matrix B is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| transC | Input | Flag for matrix C. Currently, only ACL_TRANS_N is supported. See [aclTransType](aclTransType.md) for the type definition. |
| m | Input | Number of rows of matrix A and matrix C. |
| n | Input | Number of columns of matrix B and matrix C. |
| k | Input | Number of columns of matrix A and number of rows of matrix B. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for the type definition. |
| handle | Output | Pointer to the pointer to the operator execution handle. See [aclopHandle](aclopHandle.md) for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
