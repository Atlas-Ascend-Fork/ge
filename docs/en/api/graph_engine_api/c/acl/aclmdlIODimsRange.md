# aclmdlIODimsRange

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T14:34:57.153Z pushedAt=2026-11-18T03:33:38.585Z nl=2 -->

```c
#define ACL_DIM_ENDPOINTS        2
#define ACL_MAX_DIM_CNT          128

typedef struct aclmdlIODimsRange {
    size_t rangeCount; /** number of dimensions */
    int64_t range[ACL_MAX_DIM_CNT][ACL_DIM_ENDPOINTS]; /** Two-dimensional array of shape ranges. The first dimension indicates the number of dimensions, and the second dimension indicates the upper and lower bounds of the range. */
} aclmdlIODimsRange;
```
