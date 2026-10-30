# GeSessionGraphDebugJSONPrint

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-27T16:30:13.038Z pushedAt=2026-10-28T13:32:59.059Z nl=2 -->

## Product Support

See [Session API product support](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Feature Description

In a debugging scenario, after the graph is loaded, use this API to export the execution task information in the graph in JSON format, including the stream ID, task ID, task type, and other information. Then, view the task visualization information through tracing (for example, chrome://tracing/).

If the GeSessionGraphDebugJSONPrint API is not called, you can also generate a JSON file of the execution task information by setting the following environment variables when calling [GeSessionLoadGraph](GeSessionLoadGraph.md):

```c
export DUMP_GE_GRAPH=1
```

The directory of the generated JSON file follows the rules below, and the file name is debug\_graph\_\{graph\_id\}\_\{time\}\_\{pid\}\_\{seq\}:

- If the environment variable NPU\_COLLECT\_PATH is specified, the generated JSON file is in the $\{NPU\_COLLECT\_PATH\}/extra-info/ directory.
- If the environment variable DUMP\_GRAPH\_PATH is specified, the generated JSON file is in the $\{DUMP\_GRAPH\_PATH\}/ directory.
- If no environment variable is specified, the generated JSON file is in the directory where the current script is executed.

For details about the preceding environment variables, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

## Prototype

```c
ge::Status GeSessionGraphDebugJSONPrint(ge::Session &session, uint32_t graph_id, uint32_t flags, ge::AscendString &json_result);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| session | Input | Session instance for graph loading. |
| graph_id | Input | ID of the graph to be exported. |
| flags | Input | Reserved. Currently fixed to 0. |
| json_result | Output | JSON string containing the task information of the graph. |

## Return Value

| parameter name | Type | Description |
| --- | --- | --- |
| - | Status | GE_RTI_MODEL_NOT_LOADED: The graph is not loaded.<br>SUCCESS: The information is exported successfully.<br>FAILED: Failed to export the information. |

## Constraints

- Before calling this API, determine the memory size allocated on the Device.
- Before calling this API, complete the [CompileGraph](../../cpp/ge/Session/CompileGraph.md) and [GeSessionLoadGraph](GeSessionLoadGraph.md) process.

## Examples

```c
ge::Session session;
uint32_t graph_id = 1; //Assume the graph has been loaded.
uint32_t flags = 0;
ge::AscendString json_result;
ge::Status ret = GeSessionGraphDebugJSONPrint(session, graphId, flags, json_result);
```
