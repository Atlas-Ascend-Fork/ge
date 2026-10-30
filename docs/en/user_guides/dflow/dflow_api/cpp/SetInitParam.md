# SetInitParam

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:44:58.547Z pushedAt=2026-10-29T02:15:47.374Z nl=2 -->

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
