# Deprecated API Replacement Scheme

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-27T16:09:35.386Z pushedAt=2026-10-29T04:05:10.456Z nl=2 -->

The single operator model execution API and CBLAS API described in this chapter will be deprecated in later versions. Continuing to use them may lead to undefined behavior. Use the aclnn operator API instead.

## Single Operator Model Execution Interface Replacement Scheme

The single operator model execution API mainly includes core steps such as compiling operators (for example, the aclopCompile API), loading operator model files (for example, the aclopLoad API), and executing operators (for example, the aclopExecuteV2 API). For different operators, this API set is called, but the input parameters must be constructed based on the operator's inputs, outputs, and attributes. **Therefore, when replacing the single operator model execution API, directly use the corresponding aclnn operator API.** For example, if the Add operator was previously called through the single operator model execution API, you can now use the aclnnAdd operator API instead.

## CBLAS API Replacement Scheme

CBLAS APIs provide two main types of functionality: matrix-vector multiplication and matrix-matrix multiplication. The core steps include compiling operators with the ATC tool, loading the operator model file (for example, the aclopLoad API), and executing operators (for example, aclblasGemvEx). Matrix-vector multiplication and matrix-matrix multiplication use different execution APIs, and their input and output data types also differ. **Therefore, there is no direct one-to-one mapping between CBLAS APIs and aclnn operator APIs. Replacement must be implemented by combining basic aclnn operator APIs such as Matmul, Muls, and Add. For sample code, see [Link](https://gitcode.com/cann/ge/blob/9.2.0/examples/acl/4_sample_acl_gemm/README.md).**

The specific replacement schemes are as follows:

| new/old API | CBLAS API | aclnn combination scheme |
| --- | --- | --- |
| API name | aclblasGemvEx<br>aclblasCreateHandleForGemvEx | aclnnMatmul(n=1) + aclnnMuls + aclnnAdd |
| data type | FLOAT16/FLOAT | FLOAT16/FLOAT |
| formula | y = αAx + βy | Four steps: Matmul → Muls → Muls → Add |

| new/old API | CBLAS API | aclnn combination scheme |
| --- | --- | --- |
| API name | aclblasHgemv<br>aclblasCreateHandleForHgemv | aclnnMatmul(n=1) + aclnnMuls + aclnnAdd |
| data type | FLOAT16 (fixed) | FLOAT16 (fixed) |
| formula | y = αAx + βy | Four steps: Matmul → Muls → Muls → Add |

| new/old API | CBLAS API | aclnn combination scheme |
| --- | --- | --- |
| API name | aclblasS8gemv<br>aclblasCreateHandleForS8gemv | aclnnQuantMatmulV5(n=1) + aclnnMuls + aclnnAdd |
| data type | INT8 × INT8 input → INT32 output | INT8 input → INT32 output |
| formula | y = αAx + βy | Four steps: QuantMatmulV5 → Muls(INT32) → Muls(INT32) → Add |

| new/old API | CBLAS API | aclnn combination scheme |
| --- | --- | --- |
| API name | aclblasGemmEx<br>aclblasCreateHandleForGemmEx | aclnnMatmul + aclnnMuls + aclnnAdd |
| data type | FLOAT16/FLOAT | FLOAT16/FLOAT |
| formula | C = alpha × op(A) × op(B) + beta × C | Four steps: Matmul → Muls(alpha) → Muls(beta) → Add |

| new/old API | aclblas | aclnn combination scheme |
| --- | --- | --- |
| API name | aclblasHgemm<br>aclblasCreateHandleForHgemm | aclnnMatmul + aclnnMuls + aclnnAdd |
| data type | FLOAT (fixed) | FLOAT (fixed) |
| formula | C = alpha × op(A) × op(B) + beta × C | Four steps: Matmul → Muls(alpha) → Muls(beta) → Add |

| new/old API | aclblas | aclnn combination scheme |
| --- | --- | --- |
| API name | aclblasS8gemm<br>aclblasCreateHandleForS8gemm | aclnnQuantMatmulV5 + aclnnMuls + aclnnAdd |
| data type | INT8 × INT8 input → INT32 output | INT8 input → INT32 output |
| formula | C = alpha × A × B + beta × C | Four steps: QuantMatmulV5 → Muls(INT32) → Muls(INT32) → Add |
