# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:45:06.802Z pushedAt=2026-10-28T07:40:22.383Z nl=2 -->

An ABI-safe IR attribute definition class used to define the attribute specification of a node (attribute name, attribute type, data type, and default value).

## Header File to Include

```c++
#include "compliant_node_builder.h"
```

## Public Member Functions

```c++
IrAttrDefV2()
IrAttrDefV2(const char_t *attr_name, IrAttrType ir_attr_type, const char_t *attr_data_type, const AttrValue &attr_default_value)
~IrAttrDefV2()
IrAttrDefV2(const IrAttrDefV2 &other)
IrAttrDefV2 &operator=(const IrAttrDefV2 &other)
IrAttrDefV2(IrAttrDefV2 &&other) noexcept
IrAttrDefV2 &operator=(IrAttrDefV2 &&other) noexcept

IrAttrDefV2 &AttrName(const char_t *attr_name)
IrAttrDefV2 &AttrType(IrAttrType ir_attr_type)
IrAttrDefV2 &AttrDataType(const char_t *attr_data_type)
IrAttrDefV2 &DefaultValue(const AttrValue &attr_default_value)

const char_t *GetAttrName() const
IrAttrType GetAttrType() const
const char_t *GetAttrDataType() const
const AttrValue &GetDefaultValue() const
```
