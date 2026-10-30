# registered

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:08:26.931Z pushedAt=2026-10-29T03:19:50.559Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Checks whether a message type ID has been registered.

## Prototype

```python
registered(msg_type)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| msg_type | int | Type ID to be registered. |

## Return Value

bool. True indicates that the type ID has been registered, and false indicates that it has not been registered.

## Examples

```python
import dataflow as df
registered = df.msg_type_register.registered(1026)
```

## Constraints

None
