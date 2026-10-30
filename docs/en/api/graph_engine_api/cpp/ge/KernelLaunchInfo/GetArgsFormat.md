# GetArgsFormat

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:37:06.412Z pushedAt=2026-10-28T07:40:22.824Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Obtains the **ArgsFormat** of the operator.

## Prototype

```c++
const char *GetArgsFormat() const
```

## Parameters

None

## Return Value

On success, returns the **ArgsFormat** of the operator.

On failure, returns **nullptr**.

## Constraints

Only **AI CPU** and **AI Core** tasks can obtain the **ArgsFormat**.

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
  ...
  auto aicore_task = KernelLaunchInfo::LoadFromData(context, tasks.back());
  auto aicore_args_format_str = aicore_task.GetArgsFormat();
  auto aicore_args_format = ArgsFormatSerializer::Deserialize(aicore_args_format_str);
  ...
}
```
