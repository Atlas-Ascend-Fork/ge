# synchronize

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T04:01:20.529Z pushedAt=2026-11-17T11:32:33.936Z nl=2 -->

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
