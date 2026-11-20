# aclblasCreateHandleForGemmEx

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:26:21.961Z pushedAt=2026-11-17T13:36:54.564Z nl=2 -->

## Product Support

<!-- npu="950" id148 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id148 -->
<!-- npu="A3" id149 -->
- Atlas A3 products: Supported
<!-- end id149 -->
<!-- npu="910b" id150 -->
- Atlas A2 products: Supported
<!-- end id150 -->
<!-- npu="310b" id151 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id151 -->
<!-- npu="310p" id152 -->
- Atlas inference products: Supported
<!-- end id152 -->
<!-- npu="910" id153 -->
- Atlas training products: Supported
<!-- end id153 -->
<!-- npu="IPV350" id154 -->
- IPV350: not supported
<!-- end id154 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasCreateHandleForGemmEx_res.md#id1 -->

## Function Description

Creates a handle for matrix-matrix multiplication. The data types of the input and output data are set through input parameters. After the handle is created, call [aclopExecWithHandle](aclopExecWithHandle.md) to execute the operator.

The data types of A, B, and C support only the following combinations:

| Data Type of A | Data Type of B | Data Type of C |
| --- | --- | --- |
| aclFloat16 | aclFloat16 | aclFloat16 |
| aclFloat16 | aclFloat16 | float(float32) |
| int8_t | int8_t | float(float32) |
| int8_t | int8_t | int32_t |

## Prototype

```c
aclError aclblasCreateHandleForGemmEx(aclTransType transA,
aclTransType transB,
aclTransType transC,
int m,
int n,
int k,
aclDataType dataTypeA,
aclDataType dataTypeB,
aclDataType dataTypeC,
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
| dataTypeA | Input | Data type of matrix A. See aclDataType for type definition. |
| dataTypeB | Input | Data type of matrix B. See aclDataType for the type definition. |
| dataTypeC | Input | Data type of matrix C. See aclDataType for the type definition. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for the type definition. |
| handle | Output | Pointer to a pointer to the operator execution handle. See [aclopHandle](aclopHandle.md) for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
