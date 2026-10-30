# SetInput

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:27:44.016Z pushedAt=2026-10-28T07:42:19.699Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Sets the operator Input, that is, which operator's output is connected to this operator.

The following SetInput methods are available:

To specify the 0th Output of srcOprt as the Input of the current operator, use the first prototype to set the Input of the current operator. In this case, you do not need to specify the Output name of srcOprt.

To specify another Output of srcOprt as the Input of the current operator, use the second prototype to set the Input of the current operator. In this case, you need to specify the Output name of srcOprt.

To specify another Output of srcOprt as the Input of the current operator, use the third prototype to set the Input of the current operator. In this case, you need to specify the index-th Output of srcOprt.

## Prototype

> [!NOTE]
> The APIs whose data type is string will be deprecated in later versions. You are advised to use the APIs whose data type is not string.

```c++
Operator &SetInput(const std::string &dst_name, const Operator &src_oprt)
Operator &SetInput(const char_t *dst_name, const Operator &src_oprt)
Operator &SetInput(const std::string &dst_name, const Operator &src_oprt, const std::string &name)
Operator &SetInput(const char_t *dst_name, const Operator &src_oprt, const char_t *name)
Operator &SetInput(const std::string &dst_name, const Operator &src_oprt, uint32_t index)
Operator &SetInput(const char_t *dst_name, const Operator &src_oprt, uint32_t index)
Operator &SetInput(uint32_t dst_index, const Operator &src_oprt, uint32_t src_index)
Operator &SetInput(const char_t *dst_name, uint32_t dst_index, const Operator &src_oprt, const char_t *name)
Operator &SetInput(const char_t *dst_name, uint32_t dst_index, const Operator &src_oprt)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| dst_name | Input | Name of the Input of the current operator. |
| src_oprt | Input | Input operator object whose Input name is dst_name. |
| src_index | Input | The src_index-th output of src_oprt. |
| name | Input | Output name of src_oprt. |
| index | Input | The index-th Output of src_oprt. |
| dst_index | Input | The dst_index-th dynamic input named dst_name. |

## Return Value

The dispatcher itself.

## Constraints

None
