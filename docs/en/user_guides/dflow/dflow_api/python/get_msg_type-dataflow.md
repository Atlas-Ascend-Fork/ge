# get\_msg\_type (dataflow)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:55:22.439Z pushedAt=2026-10-29T03:12:17.171Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the registered message type ID based on the type definition.

## Prototype

```python
get_msg_type(clz)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| clz | Type definition | Type definition, such as int, str, or a custom class. |

## Return Value

Registered message type ID. Returns None if the type has not been registered.

## Examples

```python
import dataflow as df
class TestClass():
    def __init__(self, name, val):
        self.name = name
        self.val = val
msg_type = df.msg_type_register.get_msg_type(TestClass)
```

## Constraints

None
