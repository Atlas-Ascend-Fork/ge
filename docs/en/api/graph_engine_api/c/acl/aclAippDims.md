# aclAippDims

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T13:25:11.369Z pushedAt=2026-11-17T13:34:39.690Z nl=2 -->

```c
typedef struct aclAippDims {
    aclmdlIODims srcDims;     // dims before model conversion, such as [1,3,150,150].
    size_t srcSize;           // input data size before model conversion.
    aclmdlIODims aippOutdims; // dims information of the internal data format.
    size_t aippOutSize;       // output data size after AIPP processing.
} aclAippDims;
```

For the definition of aclmdlIODims, see [aclmdlIODims](aclmdlIODims.md).
