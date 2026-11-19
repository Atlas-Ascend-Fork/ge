# aclTransType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-17T15:24:42.060Z pushedAt=2026-11-18T06:28:30.026Z nl=2 -->

```c
typedef enum aclTransType
{
    ACL_TRANS_N,    // No transpose. This is the default.
    ACL_TRANS_T,    // Transpose.
    ACL_TRANS_NZ,   // Internal format, which provides better API performance. Recommended use case: when an input matrix serves as a base library, perform the NZ format conversion once and then use NZ-format operators multiple times to improve performance.
    ACL_TRANS_NZ_T  // Transpose of the internal format. Currently not supported.
}aclTransType;
```
