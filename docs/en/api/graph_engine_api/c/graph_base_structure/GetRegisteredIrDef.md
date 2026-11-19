# GetRegisteredIrDef (Deprecated)

<!-- md-trans-meta sourceCommit=c8667037da9b211c925a649ce709a2239785b504 translatedAt=2026-11-17T15:51:51.473Z pushedAt=2026-11-18T06:46:48.320Z nl=2 -->

**Note: This API will be deprecated in later versions. Use [GetRegisteredIrDefV2](GetRegisteredIrDefV2.md) instead.**

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Function Description

Obtains the prototype definition information of a registered IR (Intermediate Representation) operator.

## Prototype

```c
ge::Status GetRegisteredIrDef(const char *op_type, std::vector<std::pair<ge::AscendString, ge::AscendString>> &inputs, std::vector<std::pair<ge::AscendString, ge::AscendString>> &outputs, std::vector<std::pair<ge::AscendString, ge::AscendString>> &attrs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op_type | Input | Operator type. |
| inputs | Input | Input names and types of the operator. Type Includes required/dynamic/optional. |
| outputs | Input | Output names and types of the operator. Type Includes required/dynamic. |
| attrs | Input | Attribute names and attribute types of the operator. Type Includes (consistent with existing APIs, with the VT_prefix before the data type) INT/FLOAT/STRING/BOOL/DATA_TYPE/TENSOR/NAMED_ATTRS/LIST_INT/LIST_FLOAT/LIST_STRING/LIST_BOOL/LIST_TENSOR/BYTES/LIST_LIST_INT/LIST_NAMED_ATTRS. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | SUCCESS: The query succeeded.<br>FAILED: The query failed. |

## Constraints

None
