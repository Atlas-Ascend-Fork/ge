# pull\_blocks

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:31:20.601Z pushedAt=2026-10-29T06:40:47.884Z nl=2 -->

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

In the PagedAttention scenario, pulls KV from the corresponding Prompt node to the local KV Cache in the form of a block list based on the BlocksCacheKey. This API can be called only when LLMRole is DECODER.

## Prototype

```python
pull_blocks(prompt_cache_key: BlocksCacheKey, decoder_kv_cache: KvCache, prompt_blocks: List[int], decoder_blocks: List[int], **kwargs)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| prompt_cache_key | [BlocksCacheKey](BlocksCacheKey.md) | BlocksCacheKey to be pulled. |
| decoder_kv_cache | [KvCache](KvCache-constructor.md) | Target KV cache. |
| prompt_blocks | List[int] | List of block indexes of the prompt. |
| decoder_blocks | List[int] | List of block indexes of the decode. |
| **kwargs | NA | This is the general way of writing extensible parameters of a Python function. Parameters are usually passed in the key=value format.<br>For details about the optional parameters, see Table 1. |

**Table 1**  \*\*kwargs Optional Parameter

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| src_layer_range | Optional[range] | Optional parameter, used in the layer-based KV pull scenario. Range of layers at the transfer source. Only step 1 is supported. If not set, all layers are transferred. Note that this is the layer index, not the tensor index. That is, one layer corresponds to N consecutive tensors (K/V). When allocating memory, the layout must be KV,...,KV. Other scenarios are not supported. N is the value of tensor_num_per_layer, which defaults to 2. |
| dst_layer_range | Optional[range] | Optional parameter, used in the layer-based KV pull scenario. Range of layers at the transfer destination. Only step 1 is supported. If not set, all layers are transferred. Note that this is the layer index, not the tensor index. That is, one layer corresponds to N consecutive tensors (K/V). When allocating memory, the layout must be KV,...,KV. Other scenarios are not supported. N is the value of tensor_num_per_layer, which defaults to 2. |
| tensor_num_per_layer | Optional[int] | Optional parameter, indicating the number of tensors per layer. The default value is 2, and the value range is [1, total number of tensors in the cache]. When src_layer_range or dst_layer_range is set to a non-default value, tensor_num_per_layer can keep the default value or be set to another value. If another value is set, tensor_num_per_layer must also be divisible by the total number of tensors in the current cache. |

## Examples

```python
from llm_datadist import *
...
kv_cache_manager.pull_blocks(prompt_cache_key, kv_cache, [0, 1], [2, 3])

# Example of enabling the layer_range function
kv_cache_manager.pull_blocks(prompt_cache_key, kv_cache, [0, 1], [2, 3], src_layer_range=range(2), dst_layer_range=range(2))
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

If the execution time exceeds the [sync\_kv\_timeout](sync_kv_timeout.md) configuration, an [LLMException](LLMException.md) is thrown.

## Constraints

- When the same link is used, this API and the [transfer\_cache\_async](transfer_cache_async.md) API do not support concurrent execution.
- This API cannot be called concurrently by multiple threads.
