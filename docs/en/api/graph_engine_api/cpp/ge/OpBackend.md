# OpBackend

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-16T11:22:44.108Z pushedAt=2026-10-28T07:40:22.167Z nl=2 -->

Enum of backend types for custom operators. The header file is located in the installation path of CANN software under **include/graph/custom\_op.h**.

```c++
enum class OpBackend : uint32_t {
  kDevice = 0,
  kHostCPU = 1,
};
```

The enum items are described as follows:

- **kDevice**: Device backend.
- **kHostCPU**: Host CPU backend.
