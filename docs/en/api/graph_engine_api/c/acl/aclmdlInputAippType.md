# aclmdlInputAippType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T15:24:32.937Z pushedAt=2026-10-28T13:16:07.130Z nl=2 -->

```c
typedef enum {
    ACL_DATA_WITHOUT_AIPP = 0,  // The input has no AIPP information.
    ACL_DATA_WITH_STATIC_AIPP,  // The input carries static AIPP information.
    ACL_DATA_WITH_DYNAMIC_AIPP, // The input has an associated dynamic AIPP input.
    ACL_DYNAMIC_AIPP_NODE       // The input itself is a dynamic AIPP input, used to configure dynamic AIPP parameters.
} aclmdlInputAippType;
```
