# RemoveGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:20:40.008Z pushedAt=2026-10-28T07:42:19.818Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Deletes the graph corresponding to the specified ID from the current session.

## Prototype

```c++
Status RemoveGraph(uint32_t graph_id)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the graph to be deleted. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **GE_CLI_SESS_REMOVE_FAILED**: Serialization conversion failed when deleting the graph.<br>**SUCCESS**: The graph is deleted successfully.<br>**FAILED**: Failed to delete the graph. |

## Constraints

None
