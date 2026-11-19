# get\_nowait

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:44:10.543Z pushedAt=2026-11-16T08:02:16.065Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains an element from the queue without waiting, which is functionally equivalent to get\(block=False\).

## Prototype

```python
get_nowait(self)
```

## Parameters

None

## Return Value

The data object of the corresponding type in MsgType.

## Exception Handling

Throws the queue.Empty exception when the queue is empty.

## Constraints

None
