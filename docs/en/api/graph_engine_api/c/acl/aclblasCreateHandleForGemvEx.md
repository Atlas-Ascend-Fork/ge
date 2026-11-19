# aclblasCreateHandleForGemvEx

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:26:43.442Z pushedAt=2026-11-18T02:09:46.449Z nl=2 -->

## Product Support

<!-- npu="950" id979 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id979 -->
<!-- npu="A3" id980 -->
- Atlas A3 products: Supported
<!-- end id980 -->
<!-- npu="910b" id981 -->
- Atlas A2 products: Supported
<!-- end id981 -->
<!-- npu="310b" id982 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id982 -->
<!-- npu="310p" id983 -->
- Atlas inference products: Supported
<!-- end id983 -->
<!-- npu="910" id984 -->
- Atlas training products: Supported
<!-- end id984 -->
<!-- npu="IPV350" id985 -->
- IPV350: not supported
<!-- end id985 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclblasCreateHandleForGemvEx_res.md#id1 -->

## Function Description

Creates a handle for matrix-vector multiplication, with the data types of the input and output data set through input parameters. After the handle is created, call the [aclopExecWithHandle](aclopExecWithHandle.md) API to execute the operator.

The data types of A, x, and y support only the following combinations:

| Data Type of A | Data Type of x | Data Type of y |
| --- | --- | --- |
| aclFloat16 | aclFloat16 | aclFloat16 |
| aclFloat16 | aclFloat16 | float(float32) |
| int8_t | int8_t | float(float32) |
| int8_t | int8_t | int32_t |

## Prototype

```c
aclError aclblasCreateHandleForGemvEx(aclTransType transA,
int m,
int n,
aclDataType dataTypeA,
aclDataType dataTypeX,
aclDataType dataTypeY,
aclComputeType type,
aclopHandle **handle)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| transA | Input | Whether matrix A is transposed. See [aclTransType](aclTransType.md) for the type definition. |
| m | Input | Number of rows of matrix A. Row-major order is used when storing matrix multiplication data. |
| n | Input | Number of columns of matrix A. |
| dataTypeA | Input | Data type of matrix A. See aclDataType for type definition. |
| dataTypeX | Input | Data type of vector x. See aclDataType for type definition. |
| dataTypeY | Input | Data type of vector y. See aclDataType for type definition. |
| type | Input | Computation precision, defaulting to high precision. See [aclComputeType](aclComputeType.md) for the type definition. |
| handle | Output | Pointer to a pointer to the operator execution handle. See [aclopHandle](aclopHandle.md) for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
