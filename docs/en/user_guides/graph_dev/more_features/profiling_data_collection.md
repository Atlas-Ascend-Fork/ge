# Profiling Data Collection

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-21T10:01:02.445Z pushedAt=2026-10-28T08:39:18.690Z nl=2 -->

You can collect Profiling performance data during graph loading and graph execution for performance analysis. This section provides the detailed collection methods.

## Feature Overview

<!-- npu="310b" id1 -->
Atlas 200I/500 A2 inference products do not support this feature.
<!-- end id1 -->

There are two ways to collect Profiling performance data, as shown in the following table.

**Table 1** Profiling performance data collection methods

| Sequence Number | Collection Method |
| --- | --- |
| Method 1 | Pass the option parameters through [GEInitializeV2](../../../api/graph_engine_api/cpp/ge/GeSession/GEInitializeV2.md):<br>  - ge.exec.profilingMode<br>  - ge.exec.profilingOptions<br>The performance data collected in this way is stored in the path configured by the output parameter of ge.exec.profilingOptions. |
| Method 2 | Call the following APIs to collect Profiling performance data:<br>  - [aclgrphProfInit](../../../api/graph_engine_api/cpp/ge/aclgrphProfInit.md)<br>  - [aclgrphProfFinalize](../../../api/graph_engine_api/cpp/ge/aclgrphProfFinalize.md)<br>  - [aclgrphProfCreateConfig](../../../api/graph_engine_api/cpp/ge/aclgrphProfCreateConfig.md)<br>  - [aclgrphProfDestroyConfig](../../../api/graph_engine_api/cpp/ge/aclgrphProfDestroyConfig.md)<br>  - [aclgrphProfStart](../../../api/graph_engine_api/cpp/ge/aclgrphProfStart.md)<br>  - [aclgrphProfStop](../../../api/graph_engine_api/cpp/ge/aclgrphProfStop.md)<br>If iteration trace data needs to be collected, you also need to pass the option parameter ge.exec.profilingOptions through [GEInitializeV2](../../../api/graph_engine_api/cpp/ge/GeSession/GEInitializeV2.md). The fields to be passed include training_trace/bp_point/fp_point.<br>The performance data collected in this way is stored in the path configured by the profiler_path parameter of aclgrphProfInit. |

## Configuring Before Collection

Before performing Profiling collection, you need to obtain the sample for building a graph and directly compiling and running the graph from [graph\_run](https://gitee.com/ascend/samples/tree/master/cplusplus/level1_single_api/8_graphrun/graph_run), and then perform the following operations:

1. Add the “\#include "ge/ge\_prof.h"” code at the beginning of the source file "main.cpp".
2. Add the "-lmsprofiler" field under the "LIBS" line in the compilation script "Makefile", or add the "msprofiler" field under the “target\_link\_libraries” line in "CMakeLists.txt".

The header file ge/ge\_prof.h is used to define the APIs for Profiling configuration, and the corresponding library file is libmsprofiler.so (the header file is in the "CANN software installation path/include/" directory, and the library file is in the "CANN software installation path/lib64/" directory).

## Collecting Performance Data Using Method 2

This feature is a global feature rather than a session-level feature. Once enabled, it takes effect in all sessions.

The recommended API call sequence is as follows:

![](../figures/api_call_sequence.png)

The following is an example:

```c++
  // Construct the graph. This step is omitted.
  // ...

  // Initialize GE.
  std::map<std::string, std::string> ge_options = {{"ge.socVersion", "xxx"}, {"ge.graphRunMode", "1"}};
  ge::GEInitializeV2(ge_options);

 // Configure and start Profiling.
  std::string profilerResultPath = "/home/test/prof";             // This path must be created in advance.
  uint32_t length = strlen("/home/test/prof");
  ret = ge::aclgrphProfInit(profilerResultPath.c_str(), length); // Initialize Profiling.

  // Create a Session and add a Graph
  std::map<string, string> options = {{"a", "b"}, {"c", "d"}};
  uint32_t graphId = 0;
  ge::Graph graph;
  ge::GeSession *session = new GeSession(options);
  ret = session->AddGraph(graphId, graph);

 // Configure Profiling sampling parameters
  uint32_t deviceid_list[1] = {0};
  uint32_t device_nums = 1;
  uint64_t data_type_config = ProfDataTypeConfig::kProfTaskTime | ProfDataTypeConfig::kProfAiCoreMetrics | ProfDataTypeConfig::kProfAicpu | ProfDataTypeConfig::kProfTrainingTrace | ProfDataTypeConfig::kProfHccl | ProfDataTypeConfig::kProfL2cache;
  std::vector<ge::Tensor> inputs;
  std::vector<ge::Tensor> outputs;
  ProfAicoreEvents *aicore_events = NULL;
  ProfilingAicoreMetrics aicore_metrics = ProfilingAicoreMetrics::kAicoreArithmeticUtilization;
  ge::aclgrphProfConfig *pro_config = ge::aclgrphProfCreateConfig(deviceid_list, device_nums, aicore_metrics, aicore_events, data_type_config);

  ge::aclgrphProfStart(pro_config);          // Start Profiling

  session->RunGraph(graphId, inputs, outputs);  // Execute the Graph

  ge::aclgrphProfStop(pro_config);            // Stop Profiling data collection

  // Release resources
  ge::aclgrphProfDestroyConfig(pro_config);   // Destroy the Profiling configuration

  ge::aclgrphProfFinalize();                   // End Profiling

  delete session;                                 // Release the Session
  ge::GEFinalizeV2();                               // Release GE-related resources

```
