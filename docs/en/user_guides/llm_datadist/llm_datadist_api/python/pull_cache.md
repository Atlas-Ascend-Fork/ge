# pull\_cache

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:31:51.018Z pushedAt=2026-10-29T06:41:14.444Z nl=2 -->

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

Pulls KV from the corresponding Prompt node to the local KV Cache based on the CacheKey. This API can be called only when LLMRole is DECODER.

## Prototype

```python
pull_cache(cache_key: Union[CacheKey, CacheKeyByIdAndIndex], kv_cache: KvCache, batch_index: int = 0, size: int = -1, **kwargs)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| cache_key | Union[[CacheKey](CacheKey.md), [CacheKeyByIdAndIndex](CacheKeyByIdAndIndex.md)] | CacheKey to be pulled. This CacheKey must be consistent with the CacheKey of [allocate_cache](allocate_cache.md).<br>To pull by req_id, prefix_id, and model_id, pass CacheKey.<br>To pull by cache_id and batch_index, pass CacheKeyByIdAndIndex. |
| kv_cache | [KvCache](KvCache-constructor.md) | Destination KV cache. |
| batch_index | int | batch index of the destination KV cache. The default value is 0. |
| size | int | The default value is -1.<br>Set it to an integer greater than 0 to indicate the tensor size to be pulled.<br>Or set it to -1 to indicate a full copy: the size of a single local KV minus the dst_cache_offset size. |
| **kwargs | NA | This is a common way of writing extensible parameters of a Python function. Parameters are generally passed directly in the key=value format.<br>For details about the optional parameters, see Table 1. |

**Table 1** Optional parameters of \*\*kwargs

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| src_layer_range | Optional[range] | Optional parameter, used in the layer-based KV pull scenario. Range of layers of the transfer source. Only step 1 is supported. If not set, all layers are transferred. Note that this is the layer index, not the tensor index. That is, one layer corresponds to N consecutive tensors (K/V). When allocating memory, the layout must be KV,...,KV. Other scenarios are not supported. N is the value of tensor_num_per_layer, and the default value is 2. |
| dst_layer_range | Optional[range] | Optional parameter, used in the layer-based KV pull scenario. Range of layers of the transfer destination. Only step 1 is supported. If not set, all layers are transferred. Note that this is the layer index, not the tensor index. That is, one layer corresponds to N consecutive tensors (K/V). When allocating memory, the layout must be KV,...,KV. Other scenarios are not supported. N is the value of tensor_num_per_layer, and the default value is 2. |
| src_cache_offset | Optional[int] | Set it to an integer greater than or equal to 0. Indicates that data of the size specified by size is pulled from the offset position of the src_cache tensor. |
| dst_cache_offset | Optional[int] | Set it to an integer greater than or equal to 0. Indicates that the source data is pulled to the offset start position of the dst_cache tensor. |
| tensor_num_per_layer | Optional[int] | Optional parameter, indicating the number of tensors per layer. The default value is 2, and the value range is [1, total number of tensors in the cache]. When src_layer_range or dst_layer_range is set to a non-default value, tensor_num_per_layer can keep the default value or be set to another value. When set to another value, the value of tensor_num_per_layer must also be divisible by the total number of tensors in the current cache. |

## Examples

```python
from llm_datadist import *
...
cache_keys = [CacheKey(1, req_id=1), CacheKey(1, req_id=2)]
kv_cache_manager.pull_cache(cache_keys[0], cache, 0)
# Enable the layer_range function.
kv_cache_manager.pull_cache(cache_keys[1], cache, 1, src_layer_range=range(0,2), dst_layer_range=range(2,4))
# Enable the offset function.
kv_cache_manager.pull_cache(cache_keys[1], cache, src_cache_offset=0, dst_cache_offset=0)
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

If the execution time exceeds the [sync\_kv\_timeout](sync_kv_timeout.md) configuration, an [LLMException](LLMException.md) is thrown.

## Constraints

- When the same link is used, this API cannot run concurrently with the [transfer\_cache\_async](transfer_cache_async.md) API.
- This API does not support concurrent calls.
