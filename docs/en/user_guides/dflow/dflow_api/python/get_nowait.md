# get\_nowait

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:56:14.899Z pushedAt=2026-10-29T03:12:51.787Z nl=2 -->

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
