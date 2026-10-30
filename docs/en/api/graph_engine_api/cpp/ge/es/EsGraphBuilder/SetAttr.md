# SetAttr

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-10-16T13:09:13.500Z pushedAt=2026-10-28T07:40:22.475Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the type attribute of the graph.

## Prototype

- Sets the int64 type attribute of the graph.

```c++
    Status SetAttr(const char *attr_name, int64_t value)
    ```

-   Sets the string type attribute of the graph.

    ```
    Status SetAttr(const char *attr_name, const char *value)
    ```

-   Sets the boolean type attribute of the graph.

    ```
    Status SetAttr(const char *attr_name, bool value)
    ```

## Parameters


| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_name | Input | Attribute name. |
| value | Input | Attribute value. |

## Return Value


| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Operation succeeded.<br>Others: Operation failed. |

## Constraints

None

## Examples

```c++
EsGraphBuilder builder("test_graph");
builder.SetAttr("int64_attr", static_cast<int64_t>(10));
```
