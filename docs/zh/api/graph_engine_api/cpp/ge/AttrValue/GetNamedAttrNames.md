# GetNamedAttrNames

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <graph/attr\_value.h\>
- 库文件：libgraph.so

## 功能说明

枚举当前AttrValue承载的NamedAttrs集合中的子属性名称。可用于发现可选字段，或配合 [GetNamedAttrValue](GetNamedAttrValue.md) 逐项查询字段值。

## 函数原型

```c++
graphStatus GetNamedAttrNames(std::vector<AscendString> &attr_names) const
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| attr_names | 输出 | 子属性名称列表。 |

## 返回值说明

graphStatus类型：成功，返回GRAPH\_SUCCESS；否则，返回其他非零值。

## 约束说明

无

## 调用示例

假设节点已包含名为`config`的NamedAttrs属性，以下示例通过GNode获取该属性，再查询全部子属性名称。

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
