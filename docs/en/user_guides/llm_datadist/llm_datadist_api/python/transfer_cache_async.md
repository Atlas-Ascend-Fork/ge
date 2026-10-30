# transfer\_cache\_async

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:37:45.745Z pushedAt=2026-10-29T06:47:17.387Z nl=2 -->

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

Transfers KV Cache asynchronously in a hierarchical manner.

## Prototype

```python
transfer_cache_async(src_cache: KvCache,
                     layer_synchronizer: LayerSynchronizer,
                     transfer_configs: Union[List[TransferConfig], Tuple[TransferConfig]],
                     src_block_indices: Optional[Union[List[int], Tuple[int]]] = None,
                     dst_block_indices: Optional[Union[List[int], Tuple[int]]] = None,
                     dst_block_memory_size: Optional[int] = None) -> CacheTask
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| src_cache | [KvCache](KvCache-constructor.md) | Source cache. |
| layer_synchronizer | [LayerSynchronizer](LayerSynchronizer.md) | Implementation class object of LayerSynchronizer. |
| transfer_configs | Union[List[[TransferConfig](TransferConfig.md)], Tuple[[TransferConfig](TransferConfig.md)]] | List or tuple of transfer configurations. |
| src_block_indices | Optional[Union[List[int], Tuple[int]]] | Block indices of the source cache. Set when the source cache is in the PA scenario. |
| dst_block_indices | Optional[Union[List[int], Tuple[int]]] | Block indices of the destination cache. Set when the destination cache is in the PA scenario. |
| dst_block_memory_size | Optional[int] | Memory size occupied by each block of the destination cache. Set when the destination cache is in the PA scenario. If the source cache is also in the PA scenario, this parameter can be omitted, in which case it is automatically set to the memory size occupied by each block of the source cache.<br>Setting this parameter to 0 is equivalent to omitting it. |

## Examples

```python
from llm_datadist import *
...
class LayerSynchronizerImpl(LayerSynchronizer):
    def synchronize_layer(self, layer_index: int, timeout_in_millis: Optional[int]) -> bool:
        # need to control time for transfer layer here.
        return True
num_layers = 40
dst_cluster_id = 2
# need to register decoder kv addr here.
decoder_addrs = ...
assert(len(decoder_addrs) = 2*num_layers)
transfer_config = TransferConfig(dst_cluster_id, decoder_addrs, range(0, num_layers), 0)
cache_task = kv_cache_manager.transfer_cache_async(kv_cache, LayerSynchronizerImpl(), [transfer_config])
cache_task.synchronize()
cache_task.get_results()
```

## Return Value

Returns CacheTask in normal cases.

If the input data type is incorrect, a TypeError or ValueError exception is thrown.

If the input data is invalid, an [LLMException](LLMException.md) exception is thrown.

## Constraints

- Currently, only the scenario where both src\_cache and dst\_cache are contiguous caches and the scenario where both src\_cache and dst\_cache are PA are supported.
- When the same link is used, this API and the [pull\_cache](pull_cache.md) and [pull\_blocks](pull_blocks.md) APIs do not support concurrency.
- This API does not support concurrent calls.
- In single-process multi-device mode, calling this API is not supported.
