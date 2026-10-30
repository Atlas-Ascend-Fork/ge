# ProfDataTypeConfig

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:24:10.481Z pushedAt=2026-10-28T07:40:22.170Z nl=2 -->

Profiling data type configuration file. The header file is located in **include/ge/ge\_prof.h** under the file storage path after CANN software installation.

```c++
enum ProfDataTypeConfig {
  kProfTaskTime,          = 0x0002,      // Collect operator dispatch time, operator execution time, and basic operator information data to provide more comprehensive performance analysis data.
  kProfAiCoreMetrics,     = 0x0004,      // AI Core data.
  kProfAicpu,             = 0x0008,      // AI CPU data. Not supported in the current version.
  kProfL2cache,            = 0x0010,      // L2 cache sampling data.
  kProfHccl,              = 0x0020,      // Collective communication data.
  kProfTrainingTrace,     = 0x0040,      // Iteration trace data.
  kProfTaskTimeL0,        = 0x0800,      // Collect operator dispatch time and operator execution time. Compared with kProfTaskTime, this option does not collect basic operator information data, so it incurs lower performance overhead during collection and can measure the related time data more accurately.
};
```
