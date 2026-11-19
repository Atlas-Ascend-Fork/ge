# aclgrphSetOpAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:13:25.749Z pushedAt=2026-10-28T07:40:22.132Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_ir\_build.h\>
- Library file: libge\_compiler.so

## Description

Used to support graph modification by setting node attributes.

## Prototype

```c++
graphStatus aclgrphSetOpAttr(Graph &graph, aclgrphAttrType attr_type, const char_t *cfg_path)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input/Output | As an input, it represents the graph to be modified through attributes;<br>as an output, it represents the graph after modification. |
| attr_type | Input | Enum type definition of **attr_type**:<br>**enum aclgrphAttrType { ATTR_TYPE_KEEP_DTYPE = 0, ATTR_TYPE_WEIGHT_COMPRESS }**;<br>**ATTR_TYPE_KEEP_DTYPE**: Keeps the computation precision of individual operators unchanged during model compilation.<br>**ATTR_TYPE_WEIGHT_COMPRESS**: Performs weight compression on individual operators during model compilation. |
| cfg_path | Input | Path of the configuration file. An example of the configuration file is as follows. |

Example of the configuration file:

The file format for keeping the computation precision unchanged is:

```text
Opname1
Opname2
…
```

The file format for weight compression is:

```text
Opname1;Opname2
```

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Constraints

None
