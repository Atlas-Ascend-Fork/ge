# IrAttrDefV2 Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:44:28.709Z pushedAt=2026-10-28T07:40:22.382Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

**IrAttrDefV2** constructor and destructor.

## Prototype

```c++
IrAttrDefV2()
IrAttrDefV2(const char_t *attr_name, IrAttrType ir_attr_type, const char_t *attr_data_type, const AttrValue &attr_default_value)
~IrAttrDefV2()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_name | Input | Attribute name, identifying the name of this attribute in the operator. |
| ir_attr_type | Input | Attribute type, determining whether the attribute is required or optional. |
| attr_data_type | Input | Attribute data type, specifying the data type of the attribute value. |
| attr_default_value | Input | Attribute default value, the default value of an optional attribute. |

## Return Value

Constructs and returns an **IrAttrDefV2** object.

## Constraints

None
