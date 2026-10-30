# GetRegisteredIrDefV2

<!-- md-trans-meta sourceCommit=5c51f49b6f181c898321d93e02bc7a29ce8a547e translatedAt=2026-10-27T16:35:32.426Z pushedAt=2026-10-28T13:41:39.201Z nl=2 -->

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <graph/operator\_reg.h\>
- Library file: libgraph.so

## Feature Description

Obtains the prototype definition information of a registered IR (Intermediate Representation) operator.

## Prototype

```c
ge::Status GetRegisteredIrDefV2(const char *op_type, std::vector<std::pair<ge::AscendString, ge::AscendString>> &inputs, std::vector<std::pair<ge::AscendString, ge::AscendString>> &outputs, std::vector<std::pair<ge::AscendString, ge::AscendString>> &attrs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| op_type | Input | Operator type. |
| inputs | Output | Input names and types of the operator. Type Includes required/dynamic/optional. |
| outputs | Output | Output names and types of the operator. Type Includes required/dynamic. |
| attrs | Output | Attribute names and attribute types of the operator. Type Includes (consistent with existing APIs, with the VT_prefix before the data type) INT/FLOAT/STRING/BOOL/DATA_TYPE/TENSOR/NAMED_ATTRS/LIST_INT/LIST_FLOAT/LIST_STRING/LIST_BOOL/LIST_TENSOR/BYTES/LIST_LIST_INT/LIST_NAMED_ATTRS. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | SUCCESS: The query succeeded.<br>FAILED: The query failed. |

## Constraints

None
