# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:56:42.292Z pushedAt=2026-10-28T07:40:22.435Z nl=2 -->

Represents the C-style output type of the ES API. C users only need to call the **esb\_funcs.h** API for graph construction, while C++ users call the public APIs of the **EsTensorHolder** class.

## Header File to Include

```c++
#include <es_c_tensor_holder.h>
```

## Public Member Functions

```c++
EsCTensorHolder(EsCGraphBuilder &owner, const ge::GNode &producer, int32_t index)
~EsCTensorHolder()
EsCTensorHolder(EsCTensorHolder &&) noexcept
EsCTensorHolder &operator=(EsCTensorHolder &&) noexcept
ge::Status SetDataType(const ge::DataType data_type)
ge::Status SetFormat(const ge::Format format)
ge::Status SetOriginFormat(const ge::Format format)
ge::Status SetStorageFormat(const ge::Format format)
ge::Status SetOriginShape(const ge::Shape &shape)
ge::Status SetStorageShape(const ge::Shape &shape)
ge::Status SetShape(const ge::Shape &shape)
ge::Status SetOriginSymbolShape(const char *const *shape_str, const int64_t shape_str_num)
ge::GNode &GetProducer()
int32_t GetOutIndex() const
EsCGraphBuilder &GetOwnerBuilder()
```
