# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:49:53.810Z pushedAt=2026-10-28T07:40:22.868Z nl=2 -->

A Builder pattern class for constructing graph nodes based on input/output names, allowing callers to construct nodes by specifying port names rather than positions.

## Header File to Include

```c++
#include <graph/named_io_node_builder.h>
```

## Public Member Functions

```c++
explicit NamedIoNodeBuilder(Graph &graph)
~NamedIoNodeBuilder()
NamedIoNodeBuilder &Type(const char_t *type)
NamedIoNodeBuilder &Name(const char_t *name)
NamedIoNodeBuilder &AddInput(const char_t *name)
NamedIoNodeBuilder &AddInput(const char_t *name, const TensorDesc &desc)
NamedIoNodeBuilder &AddOutput(const char_t *name)
NamedIoNodeBuilder &AddOutput(const char_t *name, const TensorDesc &desc)
NamedIoNodeBuilder &Attr(const char_t *name, const AttrValue &value)
std::unique_ptr<GNode> Build(AscendString &error_message)
```
