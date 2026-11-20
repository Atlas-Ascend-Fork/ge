# RemoveGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:53:05.377Z pushedAt=2026-10-28T07:40:22.662Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Deletes the graph corresponding to the specified ID from the current **GeSession**.

## Prototype

```c++
Status RemoveGraph(uint32_t graph_id)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the graph to be deleted. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **GE_CLI_SESS_REMOVE_FAILED**: Serialization conversion failure when deleting the graph.<br>**SUCCESS**: The graph is deleted successfully.<br>**FAILED**: Failed to delete the graph. |

## Constraints

None
