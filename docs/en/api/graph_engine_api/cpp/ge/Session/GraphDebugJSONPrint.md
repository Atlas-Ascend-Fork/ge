# GraphDebugJSONPrint

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-26T13:17:51.220Z pushedAt=2026-10-28T07:42:19.808Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

In debugging scenarios, after the graph is loaded (that is, after the [CompileGraph](CompileGraph.md) and [LoadGraph](LoadGraph.md) processes are complete), use this API to export the running task information of the graph in JSON format, including the stream ID, task ID, and task type. Then, view the task visualization information through tracing (for example, **chrome://tracing/**).

If the GraphDebugJSONPrint API is not called, you can set the following environment variables during [LoadGraph](LoadGraph.md) to also generate a JSON file containing the running task information. The method is as follows:

```c++
export DUMP_GE_GRAPH=1
```

The directory of the generated JSON file follows the rules below, and the file name is debug\_graph\_\{graph\_id\}\_\{time\}\_\{pid\}\_\{seq\}:

- If the environment variable NPU\_COLLECT\_PATH is specified, the generated JSON file is in the $\{NPU\_COLLECT\_PATH\}/extra-info/ directory.
- If the environment variable DUMP\_GRAPH\_PATH is specified, the generated JSON file is in the $\{DUMP\_GRAPH\_PATH\}/ directory.
- If no environment variable is specified, the generated JSON file is in the directory where the current script is executed.

For details about the preceding environment variables, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

## Prototype

```c++
Status GraphDebugJSONPrint(const uint32_t graph_id, uint32_t flags, AscendString &json_result) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be exported. |
| flags | Input | Reserved. Currently fixed to 0. |
| json_result | Output | JSON string containing the task information of the graph. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | GE_RTI_MODEL_NOT_LOADED: The graph is not loaded.<br>SUCCESS: The information is exported successfully.<br>FAILED: Failed to export the information. |

## Constraints

- Before calling this API, determine the memory size allocated on the Device.
- Before calling this API, complete the [CompileGraph](CompileGraph.md) and [LoadGraph](LoadGraph.md) processes.

## Examples

```c++
ge::Session session;
uint32_t graph_id = 1; // Assume the graph has been loaded.
uint32_t flags = 0;
ge::AscendString json_result;
ge::Status ret = session->GraphDebugJSONPrint(graph_id, flags, json_result);
```
