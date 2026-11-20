# ProfilingAicoreMetrics

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:24:23.257Z pushedAt=2026-10-28T07:40:22.173Z nl=2 -->

Profiling AI Core metrics. The header file is located in **include/ge/ge\_prof.h** under the file storage path after CANN software installation.

```c++
enum ProfilingAicoreMetrics {
  kAicoreArithmeticUtilization = 0,  // Statistics on the proportion of various computation metrics.
  kAicorePipeUtilization = 1,  // Proportion of time consumed by computation units and data movement units.
  kAicoreMemory = 2,  // UB/L1/L2 read/write bandwidth data.
  kAicoreMemoryL0 = 3,  // L0 read/write bandwidth data.
  kAicoreResourceConflictRatio = 4,  // Proportion of pipeline queue instructions.
  kAicoreMemoryUB = 5,   // Fine-grained UB read/write bandwidth data.
  kAicoreL2Cache = 6,   // Number of read/write cache hits and reallocations after misses.

  kAicorePipelineExecuteUtilization = 7,   // Proportion of time consumed by computation units and data movement units.
};
```

<!-- npu="950,A3,910b,310b" id1 -->
The products supported by the **kAicoreL2Cache** parameter are:

Atlas 200I/500 A2 inference products

Atlas A2 products

Atlas A3 products

Ascend 950PR&950DT products
<!-- end id1 -->

<!-- npu="310b" id2 -->
The products supported by the **kAicorePipelineExecuteUtilization** parameter are:

Atlas 200I/500 A2 inference products
<!-- end id2 -->
