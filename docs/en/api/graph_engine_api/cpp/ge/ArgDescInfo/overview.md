# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:10:23.774Z pushedAt=2026-10-28T07:40:22.237Z nl=2 -->

**ArgDescInfo** is a class used to describe the address information of the **Args** input parameters of a kernel. It semantically expresses the **Args** addresses, clarifying the purpose and related information of each address. Multiple **ArgDescInfo** objects can form an **ArgsFormat**, which is expressed in code as **std::vector<ArgDescInfo\>**.

## Header File to Include

```c++
#include <graph/arg_desc_info.h>
```

## Public Member Functions

```c++
explicit ArgDescInfo(ArgDescType arg_type, int32_t ir_index = -1, bool is_folded = false)
static ArgDescInfo CreateCustomValue(uint64_t custom_value)
static ArgDescInfo CreateHiddenInput(HiddenInputSubType hidden_type)
ArgDescType GetType() const
uint64_t GetCustomValue() const
graphStatus SetCustomValue(uint64_t custom_value)
HiddenInputSubType GetHiddenInputSubType() const
graphStatus SetHiddenInputSubType(HiddenInputSubType hidden_type)
int32_t GetIrIndex() const
void SetIrIndex(int32_t ir_index)
bool IsFolded() const
void SetFolded(bool is_folded)
```
