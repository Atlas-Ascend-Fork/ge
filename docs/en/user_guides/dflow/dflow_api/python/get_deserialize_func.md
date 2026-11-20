# get\_deserialize\_func

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:39:07.387Z pushedAt=2026-11-16T08:01:31.013Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the registered deserialization function based on the message type ID.

## Prototype

```python
get_deserialize_func(msg_type)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| msg_type | int | Type ID to be registered. |

## Return Value

Registered deserialization function. If no function is registered, None is returned.

## Examples

```python
import dataflow as df
deserialize_func = df.msg_type_register.get_deserialize_func(1026)
```

## Constraints

None
