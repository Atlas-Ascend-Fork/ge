# aclOpCompileFlag

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T15:44:14.660Z pushedAt=2026-10-28T13:16:25.276Z nl=2 -->

**This parameter is deprecated. Do not configure it, as compatibility issues may arise in later versions.**

```c
typedef enum aclCompileFlag {
    ACL_OP_COMPILE_DEFAULT,  //Precise compilation. When no compile flag is set, precise compilation is used by default.
    ACL_OP_COMPILE_FUZZ
} aclOpCompileFlag;
```
