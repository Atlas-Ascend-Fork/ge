# aclgrphParseCaffe

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:09:24.435Z pushedAt=2026-10-28T07:40:22.123Z nl=2 -->

## Applicable Products

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Not supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Not supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference products: Supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Supported
<!-- end id6 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphParseCaffe_res.md#id1 -->

## Header File/Library File

- Header file: \#include <parser/caffe\_parser.h\>
- Library file: libfmk\_parser.so

## Description

Parses a Caffe model into a graph.

## Prototype

```c++
graphStatus aclgrphParseCaffe(const char *model_file, const char *weights_file, ge::Graph &graph)
graphStatus aclgrphParseCaffe(const char *model_file, const char *weights_file, const std::map<ge::AscendString, ge::AscendString> &parser_params, ge::Graph &graph)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model_file | Input | Path to the original Caffe model file. |
| weights_file | Input | Path to the Caffe weight file. |
| parser_params | Input | Map of configuration parameters, where the key is the parameter type and the value is the parameter value, both in **AscendString** format, used to describe the original model parsing parameters.<br>For the configuration parameters supported in the map, see [Configuration Parameters Supported by the Parser Parsing API](./parser_config_parameters.md). |
| graph | Output | Graph generated after parsing. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Constraints

None
