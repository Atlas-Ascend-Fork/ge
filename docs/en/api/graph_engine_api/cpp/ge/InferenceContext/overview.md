# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:31:59.339Z pushedAt=2026-10-28T07:40:22.801Z nl=2 -->

Obtains the inference context object and sets the shapes and data types of the corresponding objects. It is mainly used for resource operators.

## Header File to Include

```c++
#include <graph/inference_context.h>
```

## Public Member Functions

```c++
void SetInputHandleShapesAndTypes(std::vector<std::vector<ShapeAndType>> &&shapes_and_types)
void SetOutputHandleShapesAndTypes(const std::vector<std::vector<ShapeAndType>> &shapes_and_types)
void SetOutputHandleShapesAndTypes(std::vector<std::vector<ShapeAndType>> &&shapes_and_types)
const std::vector<std::vector<ShapeAndType>> &GetInputHandleShapesAndTypes() const
const std::vector<std::vector<ShapeAndType>> &GetOutputHandleShapesAndTypes() const
void SetMarks(const std::vector<std::string> &marks)
void SetMarks(const std::vector<AscendString> &marks)
const std::vector<std::string> &GetMarks() const
void GetMarks(std::vector<AscendString> &marks) const
static std::unique_ptr<InferenceContext> Create(
void *resource_context_mgr = nullptr
)
ResourceContext *GetResourceContext(const ge::AscendString &key)
graphStatus SetResourceContext(const ge::AscendString &key, ResourceContext *resource_context)
graphStatus RegisterReliedOnResourceKey(const ge::AscendString &key)
graphStatus AddChangedResourceKey(const ge::AscendString &key)
const std::set<ge::AscendString>& GetReliedOnResourceKeys() const
const std::set<ge::AscendString>& GetChangedResourceKeys() const
void ClearChangedResourceKeys()
```
