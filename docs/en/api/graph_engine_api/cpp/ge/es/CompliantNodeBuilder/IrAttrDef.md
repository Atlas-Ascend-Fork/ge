# IrAttrDef

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:36:19.884Z pushedAt=2026-10-28T07:40:22.344Z nl=2 -->

This struct is used for legacy APIs and contains `std::string` fields. For new code, use the ABI-safe [IrAttrDefV2](./IrAttrDefV2/IrAttrDefV2.md) instead.

IR attribute definition structure, defined as follows:

```c++
struct IrAttrDef {
  std::string attr_name;
  IrAttrType ir_attr_type;
  std::string attr_data_type;  // see `kIrAttrTypesMap` in `operator.cc`
  AttrValue attr_default_value;
}
```

The parameters are described as follows:

| Member Name | Type | Description |
| --- | --- | --- |
| attr_name | std::string | Attribute name. |
| ir_attr_type | IrAttrType | Attribute type. |
| attr_data_type | std::string | The value is a key in kIrAttrTypesMap:<br>const std::map<std::string, std::string> kIrAttrTypesMap = {<br>   {"Int", "VT_INT"},<br>   {"Float", "VT_FLOAT"},<br>   {"String", "VT_STRING"},<br>   {"Bool", "VT_BOOL"},<br>   {"Tensor", "VT_TENSOR"},<br>   {"NamedAttrs", "VT_NAMED_ATTRS"},<br>   {"ListInt", "VT_LIST_INT"},<br>   {"ListFloat", "VT_LIST_FLOAT"},<br>   {"ListString", "VT_LIST_STRING"},<br>   {"ListBool", "VT_LIST_BOOL"},<br>   {"ListTensor", "VT_LIST_TENSOR"},<br>   {"Bytes", "VT_BYTES"},<br>   {"ListListInt", "VT_LIST_LIST_INT"},<br>   {"ListNamedAttrs", "VT_LIST_NAMED_ATTRS"},<br>   {"Type", "VT_DATA_TYPE"},<br>   {"ListType", "VT_LIST_DATA_TYPE"},<br>} |
| attr_default_value | AttrValue | Attribute default value. |
