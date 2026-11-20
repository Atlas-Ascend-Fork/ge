# aclOpCompileFlag

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T14:57:41.868Z pushedAt=2026-11-18T03:40:52.047Z nl=2 -->

**This parameter is deprecated. Do not configure it, as compatibility issues may arise in later versions.**

```c
typedef enum aclCompileFlag {
    ACL_OP_COMPILE_DEFAULT,  // Exact compilation. When no compile flag is set, exact compilation is used by default.
    ACL_OP_COMPILE_FUZZ
} aclOpCompileFlag;
```
