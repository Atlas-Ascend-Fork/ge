# aclgrphGenerateForOp

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:06:04.234Z pushedAt=2026-10-28T07:40:22.113Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_ir\_build.h\>
- Library file: libge\_compiler.so

## Description

Constructs a Graph based on single-operator information or a single-operator JSON file.

## Prototype

```c++
graphStatus aclgrphGenerateForOp(const AscendString &op_type, const std::vector<TensorDesc> &inputs, const std::vector<TensorDesc> &outputs, Graph &graph)
graphStatus aclgrphGenerateForOp(const AscendString &json_path, std::vector<Graph> &graphs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op_type | Input | Operator type. |
| inputs | Input | List of input Tensor formats of the operator. |
| outputs | Input | List of output Tensor formats of the operator. |
| json_path | Input | Path of the single-operator JSON file. |
| graph | Output | Generated Graph. |
| graphs | Output | List of generated Graphs. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Constraints

None
