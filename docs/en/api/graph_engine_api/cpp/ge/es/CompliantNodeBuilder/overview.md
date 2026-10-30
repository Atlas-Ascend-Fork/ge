# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:40:56.785Z pushedAt=2026-10-28T07:40:22.365Z nl=2 -->

**CompliantNodeBuilder** is a node builder class used to construct graph nodes that comply with the IR specification. It provides a chained API to define the IR inputs, outputs, attributes, and other information of a node, ensuring that the generated nodes meet the IR specification requirements. This class is internally invoked by the ES generated API. Graph builders who construct graphs directly based on the ES generated API do not need to be aware of this class. However, if you want to customize the ES API, you need to use the capabilities provided by this class to build nodes.

## Header File to Include

```c++
#include "compliant_node_builder.h"
```

## Public Member Functions

```c++
explicit CompliantNodeBuilder(ge::Graph *graph)
~CompliantNodeBuilder()
CompliantNodeBuilder(CompliantNodeBuilder &&) noexcept
CompliantNodeBuilder(const CompliantNodeBuilder &)
CompliantNodeBuilder &OpType(const char_t *type)
CompliantNodeBuilder &IrDefInputs(std::vector<IrInputDef> input_ir_def)
CompliantNodeBuilder &IrDefInputsV2(const IrInputDefV2 *input_ir_def, size_t input_ir_def_num);
CompliantNodeBuilder &IrDefInputsV2(std::initializer_list<IrInputDefV2> input_ir_def)
CompliantNodeBuilder &IrDefInputsV2(const std::vector<IrInputDefV2> &input_ir_def)
CompliantNodeBuilder &IrDefOutputs(std::vector<IrOutputDef> output_ir_def)
CompliantNodeBuilder &IrDefOutputsV2(const IrOutputDefV2 *output_ir_def, size_t output_ir_def_num);
CompliantNodeBuilder &IrDefOutputsV2(std::initializer_list<IrOutputDefV2> output_ir_def)
CompliantNodeBuilder &IrDefOutputsV2(const std::vector<IrOutputDefV2> &output_ir_def)
CompliantNodeBuilder &IrDefAttrs(std::vector<IrAttrDef> attr_ir_def)
CompliantNodeBuilder &IrDefAttrsV2(const IrAttrDefV2 *attr_ir_def, size_t attr_ir_def_num);
CompliantNodeBuilder &IrDefAttrsV2(std::initializer_list<IrAttrDefV2> attr_ir_def)
CompliantNodeBuilder &IrDefAttrsV2(const std::vector<IrAttrDefV2> &attr_ir_def)
CompliantNodeBuilder &Name(const char_t *name)
CompliantNodeBuilder &InstanceDynamicInputNum(const char_t *ir_name, int32_t num)
CompliantNodeBuilder &InstanceDynamicOutputNum(const char_t *ir_name, int32_t num)
CompliantNodeBuilder &InstanceOutputDataType(const char_t *name, ge::DataType data_type)
CompliantNodeBuilder &InstanceOutputShape(const char_t *name, const std::vector<int64_t> &shape)
CompliantNodeBuilder &InstanceOutputOriginShape(const char_t *name, const std::vector<int64_t> &shape)
CompliantNodeBuilder &InstanceOutputStorageShape(const char_t *name, const std::vector<int64_t> &shape)
CompliantNodeBuilder &InstanceOutputFormat(const char_t *name, ge::Format format)
CompliantNodeBuilder &InstanceOutputOriginFormat(const char_t *name, ge::Format format)
CompliantNodeBuilder &InstanceOutputStorageFormat(const char_t *name, ge::Format format)
ge::GNode Build() const
```

## External Functions

```c++
AttrValue CreateFrom(T &&t)
bool ValuesEqual(const T &a, const T &b)
bool ValuesEqual(const std::vector<T> &a, const std::vector<T> &b)
AttrValue CreateFromIfNotEqual(T &&value, typename std::decay<T>::type default_value)
graphStatus AddEdgeAndUpdatePeerDesc(Graph &graph, GNode &src_node, int32_t src_port_index, GNode &dst_node, int32_t dst_port_index)
```

## Enums

```c++
enum IrAttrType
enum IrInputType
enum IrOutputType
```

## Structs

```c++
struct IrAttrDef
struct IrInputDef
struct IrOutputDef
```

## Nested Classes

```c++
class IrInputDefV2
class IrOutputDefV2
class IrAttrDefV2
```
