# get\_serialize\_func

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:58:22.998Z pushedAt=2026-10-29T03:13:36.170Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the registered serialization function based on the message type ID.

## Prototype

```python
get_serialize_func(msg_type)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| msg_type | int | Registered type ID. |

## Return Value

Registered serialization function. If no function is registered, None is returned.

## Examples

```python
import dataflow as df
serialize_func = df.msg_type_register.get_serialize_func(1026)
```

## Constraints

None
