# GraphDebugJSONPrint

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T13:50:37.804Z pushedAt=2026-10-28T07:40:22.654Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

In a debugging scenario, after graph loading, use this API to export the execution task information in the graph in JSON format, including Stream ID, Task ID, Task Type, and other information. Then, view the task visualization information through tracing (for example, **chrome://tracing/**).

If the **GraphDebugJSONPrint** API is not called, you can also generate a JSON file of the execution task information by setting the following environment variables when calling [LoadGraph](./LoadGraph.md):

```bash
export DUMP_GE_GRAPH=1
```

The directory of the generated JSON file follows the rules below, and the file name is **debug\_graph\_\{graph\_id\}\_\{time\}\_\{pid\}\_\{seq\}**:

- If the environment variable **NPU\_COLLECT\_PATH** is specified, the generated JSON file is in the **$\{NPU\_COLLECT\_PATH\}/extra-info/** directory.
- If the environment variable **DUMP\_GRAPH\_PATH** is specified, the generated JSON file is in the **$\{DUMP\_GRAPH\_PATH\}/** directory.
- If no environment variable is specified, the generated JSON file is in the current script execution directory.

For details about the environment variables above, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

## Prototype

```c++
Status GraphDebugJSONPrint(const uint32_t graph_id, uint32_t flags, AscendString &json_result) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be exported. |
| flags | Input | Reserved. Currently fixed to 0. |
| json_result | Output | JSON format string containing the task information in the graph. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **GE_RTI_MODEL_NOT_LOADED**: The graph is not loaded.<br>**SUCCESS**: The information is exported successfully.<br>**FAILED**: Failed to export the information. |

## Constraints

- Before calling this API, determine the memory size allocated on the Device.
- Before calling this API, complete the [CompileGraph](./CompileGraph.md) and [LoadGraph](./LoadGraph.md) processes.

## Examples

```c++
ge::GeSession session;
uint32_t graph_id = 1; //Assume that the graph has been loaded.
uint32_t flags = 0;
ge::AscendString json_result;
ge::Status ret = session.GraphDebugJSONPrint(graph_id, flags, json_result);
```
