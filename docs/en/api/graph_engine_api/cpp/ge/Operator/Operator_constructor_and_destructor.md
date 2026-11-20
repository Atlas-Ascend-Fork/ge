# Operator Constructor and Destructor

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:26:11.606Z pushedAt=2026-10-28T07:42:19.695Z nl=2 -->

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Constructor and destructor of the Operator class.

## Prototype

> [!NOTE]
> APIs whose data type is string will be deprecated in later versions. Use APIs whose data type is not string instead.

```c++
Operator()
explicit Operator(const std::string &type)
explicit Operator(const char_t *type)
Operator(const std::string &name, const std::string &type)
Operator(const AscendString &name, const AscendString &type)
Operator(const char_t *name, const char_t *type)
virtual ~Operator() = default
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| type | Input | Operator type. |
| name | Input | Operator name. |

## Return Value

The Operator constructor returns an object of the Operator type.

## Constraints

None
