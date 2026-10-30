# aclmdlIODims

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T15:26:03.730Z pushedAt=2026-10-28T13:16:13.464Z nl=2 -->

```c
#define ACL_MAX_DIM_CNT          128
#define ACL_MAX_TENSOR_NAME_LEN  128
typedef struct aclmdlIODims {
    char name[ACL_MAX_TENSOR_NAME_LEN]; // tensor name
    size_t dimCount;                    // Number of dimensions in the Shape. For a scalar, the number of dimensions is 0.
    int64_t dims[ACL_MAX_DIM_CNT];      // Dimension information.
} aclmdlIODims;
```
