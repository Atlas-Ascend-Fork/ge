# LoadFromData

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:40:36.407Z pushedAt=2026-10-28T07:40:22.836Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Provides the capability to deserialize a serialized Task. It is mainly used in the **GenerateTask** function to deserialize the Task constructed by the framework and then modify its parameters.

## Prototype

```c++
static KernelLaunchInfo LoadFromData(const gert::ExeResGenerationContext *context, const std::vector<uint8_t> &data)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| context | Input | Input parameter of the **GenerateTask** function, which stores the basic information of the operator. |
| data | Input | Serialized result of **KernelLaunchInfo**. |

## Return Value

Returns the deserialized result of the **data** input parameter.

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
   ....
  // Change the ArgsFormat of the original AI Core task.
  auto aicore_task = KernelLaunchInfo::LoadFromData(context, tasks.back());
  // Obtain argsformat from the task.
  auto aicore_args_format_str = aicore_task.GetArgsFormat();
  auto aicore_args_format = ArgsFormatSerializer::Deserialize(aicore_args_format_str);
  size_t i = 0UL;
  // Find the position of the first input address.
  for (; i < aicore_args_format.size(); i++) {
    if (aicore_args_format[i].GetType() == ArgDescType::kIrInput ||
        aicore_args_format[i].GetType() == ArgDescType::kInputInstance) {
      break;
    }
  }
  // Insert a communication address before the input address.
  aicore_args_format.insert(aicore_args_format.begin() + i, ArgDescInfo::CreateHiddenInput(HiddenInputSubType::kHcom));
  // Serialize the modified argsformat and set it back to the Task.
  aicore_task.SetArgsFormat(ArgsFormatSerializer::Serialize(aicore_args_format).GetString());
  // Re-serialize and set it to the Tasks.
  tasks.back() = aicore_task.Serialize();
  return SUCCESS;
}
```
