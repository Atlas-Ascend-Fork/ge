# get

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:01:49.744Z pushedAt=2026-10-29T03:14:39.641Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains an element from the queue.

## Prototype

```python
get(self, block=True, timeout=None)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| block | Input | Whether to block the current thread. The default value is True, which means the current thread is blocked until the timeout expires or an element is obtained. If this parameter is set to False, a queue.Empty exception is thrown when the queue is empty. |
| timeout | Input | Blocking time for dequeuing from the queue, in ms. The value range is [0, 2147483647). The default value is None, which means the current thread is blocked until an element is obtained. |

## Return Value

The data object of the corresponding type in MsgType.

## Exception Handling

If block is False or get times out, a queue.Empty exception is thrown.

If redeployment or process exit occurs during dequeuing from the queue, a DfAbortException is thrown, indicating that the function execution is interrupted.

If get fails, a DfException is thrown, indicating that an exception occurs during dequeuing inside the UDF.

## Constraints

None.
