# aclgrphParseTensorFlow

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:08:57.481Z pushedAt=2026-10-28T07:40:22.120Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <parser/tensorflow\_parser.h\>
- Library file: libfmk\_parser.so

## Description

Parses TensorFlow models into Graphs.

## Prototype

```c++
graphStatus aclgrphParseTensorFlow(const char *model_file, ge::Graph &graph)
graphStatus aclgrphParseTensorFlow(const char *model_file, const std::map<ge::AscendString, ge::AscendString> &parser_params, ge::Graph &graph)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model_file | Input | Path of the original TensorFlow model file. |
| parser_params | Input | Map of configuration parameters, where the key is the parameter type and the value is the parameter value, both in **AscendString** format, used to describe the parsing parameters of the original model.<br>For the configuration parameters supported in the map, see [Configuration Parameters Supported by Parser Parsing APIs](./parser_config_parameters.md). |
| graph | Output | Graph generated after parsing. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

The Graph name of a Graph parsed by this API contains a timestamp. Therefore, calling this API multiple times results in different Graph names.
