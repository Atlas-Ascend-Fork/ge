# ReportAttachedStream

<!-- md-trans-meta sourceCommit=ff42e70ef4dd6e3da4c079832d1983d4ff97dea4 translatedAt=2026-10-28T08:17:50.867Z pushedAt=2026-10-28T08:50:45.508Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/resource\_usage\_context.h\>
- Library file: liblowering.so

## Description

Reports the list of auxiliary stream keys that this node will request at runtime through [`gert::EagerOpExecutionContext::RequestAttachedStream`](../EagerOpExecutionContext/RequestAttachedStream.md). The framework deduplicates and counts the keys across the entire model, and the statistics are displayed in a combined manner in the return value of [`CompiledGraphSummary::GetStreamNum`](../../ge/CompiledGraphSummary/GetStreamNum.md).

## Prototype

```c++
ge::graphStatus ReportAttachedStream(const std::vector<ge::AscendString> &keys)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| keys | Input | List of auxiliary stream keys. Each key cannot be an empty string. In dynamic graph scenarios, if the specific key list used by this node at runtime cannot be confirmed, report the key list for all possible cases. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | graphStatus | `GRAPH_SUCCESS(0)`: The report is successful. `GRAPH_FAILED`: An empty key exists or an internal error occurs. |

## Constraints

None

## Examples

```c++
ge::graphStatus DeclareResourceUsage(gert::ResourceUsageContext &ctx) override {
  const auto *input = ctx.GetInputTensor(0U);
  if (input == nullptr) {
    return ge::GRAPH_FAILED;
  }
  const auto dim = input->GetShape().GetStorageShape().GetDim(0U);
  if (dim <= 0) { /* -1 unknown / -2 unknown dimension / 0 empty tensor */
    // In dynamic graph scenarios, runtime information cannot be confirmed. Here, taking the 0th dimension of input as an example, report all possible keys.
    const std::vector<ge::AscendString> keys{
        ge::AscendString("my_op_aux_fixed"),
        ge::AscendString("my_op_aux_1"),
        ge::AscendString("my_op_aux_2"),
        ge::AscendString("my_op_aux_3")};
  } else {
    // Report all keys to be requested at runtime: fixed auxiliary streams + auxiliary streams bucketed by shape.
    const std::vector<ge::AscendString> keys{
        ge::AscendString("my_op_aux_fixed"),
        ge::AscendString(("my_op_aux_" + std::to_string(dim)).c_str())};
  }
  return ctx.ReportAttachedStream(keys);
}
```
