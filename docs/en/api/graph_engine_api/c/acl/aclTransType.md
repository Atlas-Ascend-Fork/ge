# aclTransType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:06:48.845Z pushedAt=2026-10-28T13:18:25.580Z nl=2 -->

```c
typedef enum aclTransType
{
    ACL_TRANS_N,    // No transpose. This is the default.
    ACL_TRANS_T,    // Transpose.
    ACL_TRANS_NZ,   // Internal format, which provides better API performance. Recommended use case: when an input matrix serves as a base library, perform a single NZ-format conversion and then use NZ-format operators multiple times to improve performance.
    ACL_TRANS_NZ_T  // Internal format transpose, currently not supported.
}aclTransType;
```
