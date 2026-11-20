# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:12:13.860Z pushedAt=2026-10-28T07:40:22.489Z nl=2 -->

A node output expression class used for C++ graph construction. It encapsulates the C tensor operation interfaces and provides various operations on node outputs, including setting datatype, format, shape, and other information, setting private attributes, and performing arithmetic operations.

## Header File to Include

```c++
#include "es_tensor_holder.h"
```

## Public Member Functions

```c++
EsTensorHolder operator+(const EsTensorHolder &other) const
EsTensorHolder operator-(const EsTensorHolder &other) const
EsTensorHolder operator*(const EsTensorHolder &other) const
EsTensorHolder operator/(const EsTensorHolder &other) const
EsTensorHolder(EsCTensorHolder *tensor) : tensor_holder_(tensor) {}
EsTensorHolder() = default;
EsTensorHolder &SetDataType(const ge::DataType data_type)
EsTensorHolder &SetFormat(const ge::Format format)
EsTensorHolder &SetShape(const std::vector<int64_t> &dims)
EsTensorHolder &SetOriginSymbolShape(const std::vector<const char *> &dims)
Status SetAttr(const char *attr_name, int64_t value)
Status SetAttr(const char *attr_name, const char *value)
Status SetAttr(const char *attr_name, bool value)
Status SetAttrForNode(const char *attr_name, int64_t value)
Status SetAttrForNode(const char *attr_name, const char *value)
Status SetAttrForNode(const char *attr_name, bool value)
EsCTensorHolder *GetCTensorHolder() const
int32_t GetProducerOutIndex() const
GNode *GetProducer
Status AddControlEdge(const std::vector<EsTensorHolder> &ctrl_ins) const
```
