# get\_msg\_type (dataflow)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:42:40.564Z pushedAt=2026-11-16T08:02:05.881Z nl=2 -->

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
| clz | Type definition | Type definition, for example, int, str, or a custom class. |

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
