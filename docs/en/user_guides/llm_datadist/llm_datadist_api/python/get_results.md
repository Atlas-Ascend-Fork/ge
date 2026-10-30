# get\_results

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:37:43.020Z pushedAt=2026-10-29T03:25:50.803Z nl=2 -->

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

Waits for all layers to complete transfer and obtains the execution result corresponding to each TransferConfig.

## Prototype

```python
get_results(timeout_in_millis: Optional[int] = None) -> List[LLMStatusCode]
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| timeout_in_millis | Optional[int] | Timeout for waiting, in milliseconds. The default value is None, indicating no timeout. |

## Examples

```python
rets = cache_task.get_results()
```

## Return Value

Normally, a list of [LLMStatusCode](LLMStatusCode.md) is returned, corresponding to the transfer result of each TransferConfig.

If the layer corresponding to a TransferConfig has not initiated a transfer, the corresponding return value is None.

If the input data type is incorrect, a TypeError or ValueError exception is thrown.

## Constraints

None
