# aclblasCreateHandleForS8gemm

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:27:49.168Z pushedAt=2026-11-17T13:45:16.034Z nl=2 -->

## Product Support

<!-- npu="950" id113 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id113 -->
<!-- npu="A3" id114 -->
- Atlas A3 products: Supported
<!-- end id114 -->
<!-- npu="910b" id115 -->
- Atlas A2 products: Supported
<!-- end id115 -->
<!-- npu="310b" id116 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id116 -->
<!-- npu="310p" id117 -->
- Atlas inference products: Supported
<!-- end id117 -->
<!-- npu="910" id118 -->
- Atlas training products: Supported
<!-- end id118 -->
<!-- npu="IPV350" id119 -->
- IPV350: not supported
<!-- end id119 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasCreateHandleForS8gemm_res.md#id1 -->

## Function Description

Creates a handle for matrix-matrix multiplication, where the input data type is int8\_t and the output data type is int32\_t. After the handle is created, call the [aclopExecWithHandle](aclopExecWithHandle.md) API to execute the operator.

## Prototype

```c
aclError aclblasCreateHandleForS8gemm(aclTransType transA,
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
| transA | Input | Whether matrix A is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| transB | Input | Flag indicating whether matrix B is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| transC | Input | Flag for matrix C. See [aclTransType](aclTransType.md) for the type definition.<br>Currently, only ACL_TRANS_N is supported. |
| m | Input | Number of rows of matrix A and matrix C. |
| n | Input | Number of columns of matrix B and matrix C. |
| k | Input | Number of columns of matrix A and number of rows of matrix B. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for the type definition. |
| handle | Output | Pointer to a pointer to the operator execution handle. See [aclopHandle](aclopHandle.md) for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
