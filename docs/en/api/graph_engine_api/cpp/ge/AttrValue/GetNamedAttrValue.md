# GetNamedAttrValue

<!-- md-trans-meta sourceCommit=26e2d2d9b2f8a6091084defeb9bea2abf4005f10 translatedAt=2026-11-17T02:56:37.457Z pushedAt=2026-11-18T02:41:08.907Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/attr\_value.h\>
- Library file: libgraph.so

## Description

Reads a sub-attribute by name from the NamedAttrs collection carried by the current AttrValue. The result is still carried by AttrValue and can be read via [GetAttrValue](GetAttrValue.md) to obtain the specific type; if the sub-attribute is also a collection, this API can be called again.

## Prototype

```c++
graphStatus GetNamedAttrValue(const AscendString &attr_name, AttrValue &attr_value) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| attr_name | Input | Sub-attribute name. |
| attr_value | Output | Sub-attribute value. |

## Return Value

graphStatus type: Returns GRAPH\_SUCCESS on success; otherwise, returns a non-zero value.

## Constraints

None

## Examples

Assume that the node already contains a NamedAttrs attribute named `config`, in which the sub-attribute `mode` is of the int64_t type. The following example obtains the collection via GNode, queries the Sub-attribute by name, and then reads its specific value.

```c++
#include <graph/attr_value.h>
#include <graph/gnode.h>
#include <cstdint>

ge::graphStatus GetConfigMode(const ge::GNode &node, int64_t &mode) {
  ge::AttrValue config;
  auto ret = node.GetAttr(ge::AscendString("config"), config);
  if (ret != ge::GRAPH_SUCCESS) {
    return ret;
  }

  ge::AttrValue attr_value;
  ret = config.GetNamedAttrValue(ge::AscendString("mode"), attr_value);
  if (ret != ge::GRAPH_SUCCESS) {
    return ret;
  }

  return attr_value.GetAttrValue(mode);
}
```
