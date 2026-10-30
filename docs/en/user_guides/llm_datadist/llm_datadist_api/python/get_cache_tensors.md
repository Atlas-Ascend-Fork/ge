# get\_cache\_tensors

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:37:13.975Z pushedAt=2026-10-29T03:25:50.801Z nl=2 -->

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

Obtains the cache tensor.

## Prototype

```python
get_cache_tensors(cache: KvCache, tensor_index: int = 0) -> List[Tensor]
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| cache | [KvCache](KvCache-constructor.md) | The KvCache where the tensor to be obtained resides. |
| tensor_index | int | The index of the tensor to be obtained in the cache. |

## Examples

```python
tensors = kv_cache_manager.get_cache_tensors(kv_cache, 0)
```

## Return Value

On success, returns List\[Tensor\].

TypeError or ValueError may be thrown for invalid parameters.

If the execution time exceeds the [sync\_kv\_timeout](sync_kv_timeout.md) configuration, an [LLMException](LLMException.md) is thrown.

## Constraints

This API does not support concurrent calls.
