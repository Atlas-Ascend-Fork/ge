# aclmdlInputAippType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T14:33:11.852Z pushedAt=2026-11-18T03:33:33.086Z nl=2 -->

```c
typedef enum {
    ACL_DATA_WITHOUT_AIPP = 0,  //this input has no AIPP information
    ACL_DATA_WITH_STATIC_AIPP,  //this input carries static AIPP information
    ACL_DATA_WITH_DYNAMIC_AIPP, //this input has an associated dynamic AIPP input
    ACL_DYNAMIC_AIPP_NODE       //this input is itself a dynamic AIPP input, used to configure dynamic AIPP parameters
} aclmdlInputAippType;
```
