# aclmdlAttrValue_t

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T13:53:28.726Z pushedAt=2026-11-18T03:15:56.760Z nl=2 -->

```c
typedef union {
    uint32_t mdlPriority; // Priority of model execution. A smaller value indicates a higher priority. The value ranges from 0 to 7, and the default value is 0.
    uint32_t reserved[8]; // Reserved.
} aclmdlAttrValue_t;
```
