# SetInitParam

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T04:00:26.432Z pushedAt=2026-11-16T06:05:22.961Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the initialization parameters of FunctionPp and returns the FunctionPp after the setting.

## Prototype

```cpp
FunctionPp &SetInitParam(const char_t *attr_name, const ge::AscendString &value)
FunctionPp &SetInitParam(const char_t *attr_name, const char_t *value)
FunctionPp &SetInitParam(const char_t *attr_name, const std::vector<ge::AscendString> &value)
FunctionPp &SetInitParam(const char_t *attr_name, const int64_t &value)
FunctionPp &SetInitParam(const char_t *attr_name, const std::vector<int64_t> &value)
FunctionPp &SetInitParam(const char_t *attr_name, const std::vector<std::vector<int64_t>> &value)
FunctionPp &SetInitParam(const char_t *attr_name, const float &value)
FunctionPp &SetInitParam(const char_t *attr_name, const std::vector<float> &value)
FunctionPp &SetInitParam(const char_t *attr_name, const bool &value)
FunctionPp &SetInitParam(const char_t *attr_name, const std::vector<bool> &value)
FunctionPp &SetInitParam(const char_t *attr_name, const ge::DataType &value)
FunctionPp &SetInitParam(const char_t *attr_name, const std::vector<ge::DataType> &value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_name | Input | Name of the initialization parameter. |
| value | Input | Value of the initialization parameter. |

## Return Value

Returns the configured FunctionPp.

## Exception Handling

None.

## Constraints

None.
