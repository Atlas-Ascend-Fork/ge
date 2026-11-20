# Serialize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:13:37.690Z pushedAt=2026-10-28T07:40:22.251Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: libgraph.so

## Description

Serializes **ArgsFormat** information into a data stream.

## Prototype

```c++
static AscendString Serialize(const std::vector<ArgDescInfo> &args_format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| args_format | Input | **ArgsFormat** information, which consists of several [ArgDescInfo](../ArgDescInfo/ArgDescInfo.md) objects. |

## Return Value

Returns the serialization result of **ArgsFormat**.

Returns an empty string on failure.

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context, std::vector<std::vector<uint8_t>> &tasks) {
....
// Change the ArgsFormat of the original AI Core task.
  auto aicore_task = KernelLaunchInfo::LoadFromData(context, tasks.back());
  auto aicore_args_format_str = aicore_task.GetArgsFormat();
  auto aicore_args_format = ArgsFormatSerializer::Deserialize(aicore_args_format_str);
  size_t i = 0UL;
  for (; i < aicore_args_format.size(); i++) {
    if (aicore_args_format[i].GetType() == ArgDescType::kIrInput ||
        aicore_args_format[i].GetType() == ArgDescType::kInputInstance) {
      break;
    }
  }
  aicore_args_format.insert(aicore_args_format.begin() + i, ArgDescInfo::CreateHiddenInput(HiddenInputSubType::kHcom));
  aicore_task.SetArgsFormat(ArgsFormatSerializer::Serialize(aicore_args_format).GetString());
  tasks.back() = aicore_task.Serialize();
  return SUCCESS;
}
```
