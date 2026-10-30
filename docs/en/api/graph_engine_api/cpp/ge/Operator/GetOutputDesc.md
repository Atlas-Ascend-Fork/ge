# GetOutputDesc

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-26T12:22:56.736Z pushedAt=2026-10-28T07:42:19.680Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the TensorDesc of the operator Output by the operator Output name or Output index.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. You are advised to use the API with a non-string data type.

```c++
TensorDesc GetOutputDesc(const std::string &name) const
TensorDesc GetOutputDescByName(const char_t *name) const
TensorDesc GetOutputDesc(uint32_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Operator Output name.<br>If no such operator Output name exists, a default-constructed TensorDesc object is returned, in which [DataType](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md) is mainly set to DT_FLOAT (indicating the float type) and [Format](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md) is mainly set to FORMAT_NCHW (indicating NCHW). |
| index | Input | Operator Output index.<br>If no such operator Output index exists, a default-constructed TensorDesc object is returned, in which [DataType](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md) is mainly set to DT_FLOAT (indicating the float type) and [Format](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md) is mainly set to FORMAT_NCHW (indicating NCHW). |

## Return Value

**TensorDesc** of the operator Output.

## Constraints

None
