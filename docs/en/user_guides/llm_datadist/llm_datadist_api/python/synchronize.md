# synchronize

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:35:49.627Z pushedAt=2026-10-29T06:44:15.185Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 inference products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 training products: Not supported
<!-- end id3 -->

## Function

Waits for all layers to complete transfer and obtains the overall execution result.

## Prototype

```python
synchronize(timeout_in_millis: Optional[int] = None) -> LLMStatusCode
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| timeout_in_millis | Optional[int] | Timeout for waiting, in milliseconds. The default value is None, indicating no timeout. |

## Examples

```python
ret = cache_task.synchronize()
```

## Return Value

Under normal circumstances, [LLMStatusCode](LLMStatusCode.md) is returned.

If the input data type is incorrect, a TypeError or ValueError exception is thrown.

## Constraints

None
