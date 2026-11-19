# GetNamedAttrNames

<!-- md-trans-meta sourceCommit=26e2d2d9b2f8a6091084defeb9bea2abf4005f10 translatedAt=2026-11-17T02:56:07.750Z pushedAt=2026-11-18T02:41:08.906Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/attr\_value.h\>
- Library file: libgraph.so

## Description

Enumerates the sub-attribute names in the NamedAttrs set carried by the current AttrValue. It can be used to discover optional fields, or to query field values one by one in conjunction with [GetNamedAttrValue](GetNamedAttrValue.md).

## Prototype

```c++
graphStatus GetNamedAttrNames(std::vector<AscendString> &attr_names) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| attr_names | Output | Sub-attribute name list. |

## Return Value

graphStatus type: Returns GRAPH\_SUCCESS on success; otherwise, returns a non-zero value.

## Constraints

None

## Examples

Assume that the node already contains a NamedAttrs attribute named `config`. The following example obtains this attribute through GNode and then queries all sub-attribute names.

```c++
#include <graph/attr_value.h>
#include <graph/gnode.h>
#include <vector>

ge::graphStatus GetConfigAttrNames(const ge::GNode &node,
                                 std::vector<ge::AscendString> &attr_names) {
  ge::AttrValue config;
  const auto ret = node.GetAttr(ge::AscendString("config"), config);
  if (ret != ge::GRAPH_SUCCESS) {
    return ret;
  }

  return config.GetNamedAttrNames(attr_names);
}
```
