# Shape and Memory Size Calculation Formulas for Matrices A, B, and C

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:28:16.759Z pushedAt=2026-10-29T02:01:22.701Z nl=2 -->

- If the transpose flag of matrices A, B, and C is set to ACL\_TRANS\_N or ACL\_TRANS\_T, when you apply for memory to store the data of matrices A, B, and C, the actually allocated memory must match the actual data size. The shape and memory size calculation formulas are as follows:
    - Matrix A: shape = \(m, k\); memory size = m\*k\*sizeof\(dataTypeA\)
    - Matrix B: shape = \(k, n\); memory size = k\*n\*sizeof\(dataTypeB\)
    - Matrix C: shape = \(m, n\); memory size = m\*n\*sizeof\(dataTypeC\)

- If the transpose flag of matrices A, B, and C is set to ACL\_TRANS\_NZ, it indicates that the internal data format is used, and the matrix shape is 4-dimensional. The shape and memory size calculation formulas are as follows (assuming m, k, and n are the original axes, and ⌈ ⌉ indicates align up):
    - When the data type of matrices A and B is aclFloat16, when calculating the actual memory size, m, k, and n are all aligned up to 16 for rounding calculation:
        - Matrix A: shape = \(⌈k/16⌉, ⌈m/16⌉, 16, 16\); memory size = ⌈m/16⌉\*16\*⌈k/16⌉\*16\*sizeof\(dataTypeA\)
        - Matrix B: shape = \(⌈n/16⌉, ⌈k/16⌉, 16, 16\); memory size = ⌈k/16⌉\*16\*⌈n/16⌉\*16\*sizeof\(dataTypeB\)
        - Matrix C: shape = \(⌈n/16⌉, ⌈m/16⌉, 16, 16\); memory size = ⌈m/16⌉\*16\*⌈n/16⌉\*16\*sizeof\(dataTypeC\)

    - When the data type in matrix A and matrix B is int8\_t, when calculating the actual memory size, the reduce axis is aligned up to 32 for rounding calculation, and the non-reduce axis is aligned up to 16 for rounding calculation:
        - Matrix A: shape = \(⌈k/32⌉, ⌈m/16⌉, 16, 32\); memory size = ⌈m/16⌉\*16\*⌈k/32⌉\*32\*sizeof\(dataTypeA\)
        - Matrix B: shape = \(⌈k/32⌉, ⌈n/16⌉, 32, 16\); memory size = ⌈k/32⌉\*32\*⌈n/16⌉\*16\*sizeof\(dataTypeB\)
        - Matrix C: shape = \(⌈n/16⌉, ⌈m/16⌉, 16, 16\); memory size = ⌈m/16⌉\*16\*⌈n/16⌉\*16\*sizeof\(dataTypeC\)
