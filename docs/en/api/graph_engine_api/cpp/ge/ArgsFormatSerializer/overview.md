# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:13:39.457Z pushedAt=2026-10-28T07:40:22.248Z nl=2 -->

Used to serialize and deserialize **ArgsFormat**.

## Required Header File

```c++
#include <graph/arg_desc_info.h>
```

## Public Member Functions

```c++
static AscendString Serialize(const std::vector<ArgDescInfo> &args_format)
static std::vector<ArgDescInfo> Deserialize(const AscendString &args_str)
```
