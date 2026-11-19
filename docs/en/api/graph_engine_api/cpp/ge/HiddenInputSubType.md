# HiddenInputSubType

<!-- md-trans-meta sourceCommit=c38756906bc8995090581c397cd60a0f922bfd78 translatedAt=2026-11-17T03:09:33.300Z pushedAt=2026-11-18T02:41:08.948Z nl=2 -->

Type of the hidden input address in **ArgDescInfo**. The header file is located in **include/graph/arg\_desc\_info.h** under the file storage path after CANN software installation.

```c++
enum class HiddenInputSubType {
  kHcom, // hiddenInput used for communication, used by the MC2 operator.
  kHcclSuperKernel, // HCCL superkernel hidden input, used by the Hcom collective communication operator superkernel fused task.
  kEnd
};
```
