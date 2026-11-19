# SetAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:12:51.819Z pushedAt=2026-10-28T07:40:22.490Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the private attribute of the **TensorHolder**.

## Prototype

```c++
Status SetAttr(const char *attr_name, int64_t value)
Status SetAttr(const char *attr_name, const char *value)
Status SetAttr(const char *attr_name, bool value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_name | Output | Attribute name. |
| value | Input | Attribute value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None

## Examples

```c++
  auto builder = std::make_unique<ge::EsGraphBuilder>("graph");
  auto tensor0 = builder->CreateInput(0);
  (void) tensor0.SetAttr("attr_int64", int64_t(123)); // Set the private attribute of the output tensor of the input node, with the attribute name "attr_int64" and the attribute value 123.
```
