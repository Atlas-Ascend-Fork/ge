# get\_results

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:48:17.590Z pushedAt=2026-11-17T10:35:39.361Z nl=2 -->

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
