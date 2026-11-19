# registered

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:18:42.730Z pushedAt=2026-11-17T03:39:12.016Z nl=2 -->

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

Boolean value. **True** indicates that the type ID has been registered, and **False** indicates the opposite.

## Examples

```python
import dataflow as df
registered = df.msg_type_register.registered(1026)
```

## Constraints

None
