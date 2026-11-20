# GetOption

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:12:52.888Z pushedAt=2026-10-28T08:50:45.485Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/op\_compile\_context.h\>
- Library file: liblowering.so

## Description

Obtains the value of a configuration option in the compile-time graph context by querying the corresponding option value through option\_key and outputting it. This is used to retrieve global configuration parameters (such as compilation optimization options) during operator compilation.

## Prototype

```c++
ge::graphStatus GetOption(const ge::AscendString &option_key, ge::AscendString &option) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| option_key | Input | Key name of the configuration option, used to specify the type of option to query. |
| option | Output | Value of the configuration option. The function writes the query result to this parameter. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Constraints

None
