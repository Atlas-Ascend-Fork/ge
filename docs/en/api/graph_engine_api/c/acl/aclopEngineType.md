# aclopEngineType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T15:47:44.271Z pushedAt=2026-10-28T13:16:34.282Z nl=2 -->

```c
typedef enum aclEngineType {
   ACL_ENGINE_SYS,      // Use this option when you do not care about the specific execution engine. Currently, only this option is supported.
   ACL_ENGINE_AICORE,   // Compile the operator into an AI Core operator. Currently not supported.
   ACL_ENGINE_VECTOR   // Compile the operator into a Vector Core operator. Currently not supported.
} aclopEngineType;
```
