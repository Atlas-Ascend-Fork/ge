# get\_size\_func

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:59:08.235Z pushedAt=2026-10-29T03:13:46.186Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the registered function for computing the serialized memory size based on the message type ID.

## Prototype

```python
get_size_func(msg_type)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| msg_type | int | Registered type ID. |

## Return Value

Registered function for computing the serialized memory size. Returns None if no function has been registered.

## Examples

```python
import dataflow as df
size_func = df.msg_type_register.get_size_func(1026)
```

## Constraints

None
