# aclgrphParseONNXFromMem

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:07:54.576Z pushedAt=2026-10-28T07:40:22.119Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <parser/onnx\_parser.h\>
- Library file: libfmk\_onnx\_parser.so

## Description

Parses the ONNX model loaded into memory into a **Graph**.

## Prototype

```c++
graphStatus aclgrphParseONNXFromMem(const char *buffer, size_t size, const std::map<ge::AscendString, ge::AscendString> &parser_params, ge::Graph &graph)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| buffer | Input | Pointer to the memory buffer where the ONNX model is loaded in binary form and has not yet been deserialized. |
| size | Input | Size of the memory buffer. |
| parser_params | Input | Map of configuration parameters, where the key is the parameter type and the value is the parameter value, both in **AscendString** format, used to describe the parsing parameters of the original model.<br>For the configuration parameters supported in the map, see [Configuration Parameters Supported by Parser Parsing APIs](./parser_config_parameters.md). |
| graph | Output | **Graph** generated after parsing. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
