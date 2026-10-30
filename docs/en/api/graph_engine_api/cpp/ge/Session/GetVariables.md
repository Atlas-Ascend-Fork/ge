# GetVariables

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:17:25.749Z pushedAt=2026-10-28T07:42:19.807Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Variable query API. Obtains the tensor content of all variable operators or a specified variable operator in a session.

## Prototype

> [!NOTE]
> APIs whose data type is string will be deprecated in later versions. Use APIs whose data type is not string instead.

```c++
Status GetVariables(const std::vector<std::string> &var_names, std::vector<Tensor> &var_values)
Status GetVariables(const std::vector<AscendString> &var_names, std::vector<Tensor> &var_values)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| var_names | Input | Name of the variable operator. If empty, returns the values of all variable-type operators. |
| var_values | Output | Value of the variable operator, returned in tensor format and in the same sequence as var_names. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Success.<br>Others: Scenarios such as no variable operator with the specified name exists, or the variable has not been initialized. |

## Constraints

None
