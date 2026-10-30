# register

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:08:03.729Z pushedAt=2026-10-29T03:19:43.829Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Registers the functions for serialization, deserialization, and size calculation of a custom type. It can be used together with the feed and fetch APIs to feed/fetch any Python type.

## Prototype

```python
register(msg_type, clz, serialize_func, deserialize_func, size_func=None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| msg_type | int | ID of the registered type. |
| clz | Type definition | Type definition, for example, int, str, or a custom class. |
| serialize_func | function | Serialization function. The input is any Python object, and the output is data of the bytes type, that is, the byte stream after the object is serialized. |
| deserialize_func | function | Deserialization function. The input type is bytes, indicating the byte stream to be deserialized, and the output is the deserialized object, which can be of any Python object type. |
| size_func | function | Function that calculates the memory size after serialization, in bytes. Reserved field. |

## Return Value

Returns None in normal scenarios.

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import cloudpickle
import dataflow as df

class TestClass():
    def __init__(self, name, val):
        self.name = name
        self.val = val

df.msg_type_register.register(1026, TestClass, lambda obj: cloudpickle.dumps(obj), lambda buffer: cloudpickle.loads(buffer))
```

## Constraints

None
