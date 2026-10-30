# aclmdlAttrValue_t

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:51:00.907Z pushedAt=2026-10-28T13:15:46.127Z nl=2 -->

```c
typedef union {
    uint32_t mdlPriority; // Model execution priority. A smaller value indicates a higher priority. The value ranges from 0 to 7, and the default value is 0.
    uint32_t reserved[8]; // Reserved.
} aclmdlAttrValue_t;
```
