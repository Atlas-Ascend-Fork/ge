# PullKvBlocks

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:58:58.675Z pushedAt=2026-11-17T06:35:21.660Z nl=2 -->

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

In the blocks cache transfer scenario, pulls the cache from a remote node to the local cache by using a block list. This API can be called only when the role is Decoder.

## Prototype

```cpp
Status PullKvBlocks(const CacheIndex &src_cache_index,
                    const Cache &dst_cache,
                    const std::vector<uint64_t> &src_blocks,
                    const std::vector<uint64_t> &dst_blocks,
                    const KvCacheExtParam &ext_param = {})
```

## Parameters

| Parameter | Input/Output | Value Description |
| --- | --- | --- |
| src_cache_index | Input | Index of the remote source Cache. |
| dst_cache | Input | Local destination Cache. |
| src_blocks | Input | Block index list of the source Cache. |
| dst_blocks | Input | Block index list of the destination Cache. |
| ext_param | Input | Currently, the difference between **second** and **first** in **src_layer_range** of **ext_param** must be the same as the difference between **second** and **first** in **dst_layer_range**. The default values of **first** and **second** in both **src_layer_range** and **dst_layer_range** are **–1**, indicating all layers. The value range of both is [0, maximum available layer index], and **first** must be less than or equal to **second**. The maximum available layer index is calculated as follows.<br>(CacheDesc::num_tensors / KvCacheExtParam::tensor_num_per_layer) - 1<br>Currently, **tensor_num_per_layer** supports a value range of [1, total number of tensors in the current cache], with a default value of **2**. When **src_layer_range** or **dst_layer_range** uses a non-default value, **tensor_num_per_layer** can use the default value or be set to another value. If another value is set, **tensor_num_per_layer** must also be divisible by the total number of tensors in the current cache. |

## Examples

```cpp
CacheIndex cache_key{0, 1, 0};
std::vector<uint64_t> prompt_blocks = {1,3,5,7};
std::vector<uint64_t> decoder_blocks = {0,2,4,6};
auto ret = llm_datadist.PullKvBlocks(cache_key,
                                     kv_cache,
                                     prompt_blocks,
                                     decoder_blocks);
```

## Return Value

- LLM\_SUCCESS: Success
- LLM\_PARAM\_INVALID: Parameter error
- LLM\_NOT\_YET\_LINK: No link established with the remote cluster
- LLM\_TIMEOUT: Pull timeout
- LLM\_KV\_CACHE\_NOT\_EXIST: The remote KV Cache does not exist
- Others: Failure

## Constraints

Before calling this API, call [Initialize](Initialize.md) to complete initialization. dst\_cache must be a Cache that has been allocated by the [AllocateCache](AllocateCache.md) API.
