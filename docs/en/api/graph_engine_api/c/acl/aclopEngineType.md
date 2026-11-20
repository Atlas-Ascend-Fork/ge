# aclopEngineType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:01:50.382Z pushedAt=2026-11-18T03:41:55.244Z nl=2 -->

```c
typedef enum aclEngineType {
   ACL_ENGINE_SYS,      // Fill this in when the specific execution engine is not a concern. Currently only this option is supported.
   ACL_ENGINE_AICORE,   // Build the operator into an AI Core operator. Currently not supported.
   ACL_ENGINE_VECTOR   // Build the operator into a Vector Core operator. Currently not supported.
} aclopEngineType;
```
