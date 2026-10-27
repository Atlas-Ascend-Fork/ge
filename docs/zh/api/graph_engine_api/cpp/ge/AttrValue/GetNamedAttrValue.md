# GetNamedAttrValue

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <graph/attr\_value.h\>
- 库文件：libgraph.so

## 功能说明

从当前AttrValue承载的NamedAttrs集合中，按名称读取一个子属性。结果仍由AttrValue承载，可通过 [GetAttrValue](GetAttrValue.md) 读取具体类型；若子属性也是集合，可继续调用本接口。

## 函数原型

```c++
graphStatus GetNamedAttrValue(const AscendString &attr_name, AttrValue &attr_value) const
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| attr_name | 输入 | 子属性名称。 |
| attr_value | 输出 | 子属性值。 |

## 返回值说明

graphStatus类型：成功，返回GRAPH\_SUCCESS；否则，返回其他非零值。

## 约束说明

无

## 调用示例

假设节点已包含名为`config`的NamedAttrs属性，其中子属性`mode`的类型为int64_t。以下示例通过GNode获取集合，按名称查询子属性，再读取其具体数值。

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
