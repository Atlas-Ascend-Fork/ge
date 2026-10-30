# aclAippDims

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:28:11.760Z pushedAt=2026-10-29T02:01:36.908Z nl=2 -->

```c
typedef struct aclAippDims {
    aclmdlIODims srcDims;     // dims before model conversion, for example, [1,3,150,150].
    size_t srcSize;           // Size of the input data before model conversion.
    aclmdlIODims aippOutdims; // dims information of the internal data format.
    size_t aippOutSize;       // Size of the output data after AIPP processing.
} aclAippDims;
```

For the definition of aclmdlIODims, see [aclmdlIODims](aclmdlIODims.md).
