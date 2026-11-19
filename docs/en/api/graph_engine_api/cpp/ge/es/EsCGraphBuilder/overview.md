# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:54:49.472Z pushedAt=2026-10-28T07:40:22.425Z nl=2 -->

A C-style graph builder structure, which is the internal implementation class for both C and C++. C users can build graphs by calling the esb\_funcs.h interface, while C++ users can call the public interfaces of the **EsGraphBuilder** class.

## Header File to Include

```c++
#include <es_c_graph_builder.h>
```

## Public Member Functions

```c++
EsCGraphBuilder()
explicit EsCGraphBuilder(const char *name)
~EsCGraphBuilder()
EsCGraphBuilder(EsCGraphBuilder &&) noexcept
EsCGraphBuilder &operator=(EsCGraphBuilder &&) noexcept
EsCTensorHolder *GetTensorHolderFromNode(const ge::GNode &node, int32_t output_index)
EsCTensorHolder *AppendGraphInput(const ge::char_t *name = nullptr, const ge::char_t *type = nullptr)
EsCTensorHolder *AddGraphInput(int32_t index, const ge::char_t *name = nullptr, const ge::char_t *type = nullptr, C_DataType data_type = C_DT_FLOAT, C_Format format = C_FORMAT_ND, const int64_t *dims = nullptr, int64_t dim_num = 0)
ge::Status SetGraphOutput(EsCTensorHolder *tensor, int64_t output_index)
ge::Graph *GetGraph()
std::unique_ptr<ge::Graph> BuildGraphAndReset()
ge::AscendString GenerateNodeName(const ge::char_t *node_type)
std::vector<EsCTensorHolder *> *CreateDynamicTensorHolderFromNode(const ge::GNode& node, int32_t start_idx, int32_t output_num)
T *AddResource(std::unique_ptr<T> value, std::function<void(void*)> deleter = DefaultDeleter<T>)
```
