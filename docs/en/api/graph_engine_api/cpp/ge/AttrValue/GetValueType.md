# GetValueType

<!-- md-trans-meta sourceCommit=26e2d2d9b2f8a6091084defeb9bea2abf4005f10 translatedAt=2026-11-17T02:56:46.420Z pushedAt=2026-11-18T02:41:08.915Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/attr_value.h\>
- Library file: libgraph.so

## Description

Obtains the value type currently held by `AttrValue`.

## Prototype

```c++
AttrType GetValueType() const
```

## Parameters

None

## Return Value

Returns `AttrType`. The supported enum values include:

| Enum Value | Corresponding Type |
| --- | --- |
| `AT_NONE` | Type not set or unsupported |
| `AT_INT` | `int64_t` |
| `AT_FLOAT` | `float32_t` |
| `AT_STRING` | `AscendString` |
| `AT_BOOL` | `bool` |
| `AT_TENSOR` | `Tensor` |
| `AT_DATA_TYPE` | `ge::DataType` |
| `AT_LIST_INT` | `std::vector<int64_t>` |
| `AT_LIST_FLOAT` | `std::vector<float32_t>` |
| `AT_LIST_STRING` | `std::vector<AscendString>` |
| `AT_LIST_BOOL` | `std::vector<bool>` |
| `AT_LIST_TENSOR` | `std::vector<Tensor>` |
| `AT_LIST_LIST_INT` | `std::vector<std::vector<int64_t>>` |
| `AT_LIST_DATA_TYPE` | `std::vector<ge::DataType>` |
| `AT_NAMED_ATTRS` | `ge::NamedAttrs` |

## Constraints

Enum values can only be appended at the end. When using `switch` for judgment, a `default` branch must be included.

## Examples

```c++
ge::AttrValue value;
value.SetAttrValue(static_cast<int64_t>(42));
if (value.GetValueType() == ge::AttrValue::AT_INT) {
  int64_t int_value = 0;
  value.GetAttrValue(int_value);
}
```
