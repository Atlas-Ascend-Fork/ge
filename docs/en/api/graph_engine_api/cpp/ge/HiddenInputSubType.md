# HiddenInputSubType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:19:21.427Z pushedAt=2026-10-28T07:40:22.151Z nl=2 -->

Type of the hidden input address in **ArgDescInfo**. The header file is located in **include/graph/arg\_desc\_info.h** under the file storage path after CANN software installation.

```c++
enum class HiddenInputSubType {
  kHcom, // hiddenInput used for communication, used by the MC2 operator.
  kEnd
};
```
