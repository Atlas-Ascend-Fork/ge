# GetOutputIndexByName

<!-- md-trans-meta sourceCommit=d27d43178bacf487682b31d73551eb8765c83101 translatedAt=2026-11-17T03:00:07.374Z pushedAt=2026-11-18T02:41:08.923Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the output port index by the port name of the operator.

## Prototype

```c++
graphStatus GetOutputIndexByName(const AscendString &name, int32_t &index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Port name of the operator. |
| index | Output | Output port index of the operator. If the port name does not exist, the call still returns `GRAPH_SUCCESS`, and this parameter is `-1`. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
