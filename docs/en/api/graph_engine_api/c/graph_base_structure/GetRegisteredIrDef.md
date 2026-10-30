# GetRegisteredIrDef (Deprecated)

<!-- md-trans-meta sourceCommit=5c51f49b6f181c898321d93e02bc7a29ce8a547e translatedAt=2026-10-27T16:35:07.675Z pushedAt=2026-10-28T13:38:30.272Z nl=2 -->

**Note: This API will be deprecated in later versions. Use [GetRegisteredIrDefV2](GetRegisteredIrDefV2.md) instead.**

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Feature Description

Obtains the registered IR (Intermediate Representation) operator prototype definition information.

## Prototype

```c
ge::Status GetRegisteredIrDef(const char *op_type, std::vector<std::pair<ge::AscendString, ge::AscendString>> &inputs, std::vector<std::pair<ge::AscendString, ge::AscendString>> &outputs, std::vector<std::pair<ge::AscendString, ge::AscendString>> &attrs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op_type | Input | Operator type. |
| inputs | Input | Input names and types of the operator. Types include required/dynamic/optional. |
| outputs | Input | Output names and types of the operator. Types include required/dynamic. |
| attrs | Input | Attribute names and types of the operator. Types include (consistent with existing APIs, with the VT_prefix before data types) INT/FLOAT/STRING/BOOL/DATA_TYPE/TENSOR/NAMED_ATTRS/LIST_INT/LIST_FLOAT/LIST_STRING/LIST_BOOL/LIST_TENSOR/BYTES/LIST_LIST_INT/LIST_NAMED_ATTRS. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | SUCCESS: The query succeeds.<br>FAILED: The query fails. |

## Constraints

None
