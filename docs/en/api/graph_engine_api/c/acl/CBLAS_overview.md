# CBLAS API Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:08:02.131Z pushedAt=2026-10-29T04:02:37.481Z nl=2 -->

**Note: The CBLAS APIs in this chapter will be deprecated in later versions. For replacement solutions, see [Deprecated API Replacement Solutions](deprecated_APIs_replacement_scheme.md).**

Call the APIs in this chapter to perform matrix-matrix or matrix-vector multiplication. These APIs internally encapsulate the built-in matrix multiplication operator GEMM.

Before using the APIs in this chapter, use the ATC tool to compile the operator description information of the built-in matrix multiplication operator GEMM (including input/output tensor descriptions, operator attributes, and so on) into an om model file, which is used to verify the running result of the matrix multiplication operator GEMM.
