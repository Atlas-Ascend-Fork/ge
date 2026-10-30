# PushKvBlocks

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:23:00.960Z pushedAt=2026-10-29T03:25:50.750Z nl=2 -->

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

Pushes Cache from the local node to the remote node by configuring a block list. This API can be called only when the role is Prompt.

## Prototype

```cpp
Status PushKvBlocks(const Cache &src_cache,
                    const CacheIndex &dst_cache_index,
                    const std::vector<uint64_t> &src_blocks,
                    const std::vector<uint64_t> &dst_blocks,
                    const KvCacheExtParam &ext_param = {});
```

## Parameters

| Parameter | Input/Output | Value Description |
| --- | --- | --- |
| src_cache | Input | Local source Cache. |
| dst_cache_index | Input | Index of the destination Cache on the remote node. |
| src_blocks | Input | Block index list of the source Cache. |
| dst_blocks | Input | Block index list of the destination Cache. |
| ext_param | Input | Currently, the difference between second and first in src_layer_range of ext_param must be the same as the difference between second and first in dst_layer_range. The default values of first and second in both src_layer_range and dst_layer_range are -1, indicating all layers. The value range of both is [0, maximum available layer index], and first must be less than or equal to second. The maximum available layer index is calculated as follows.<br>(CacheDesc::num_tensors / KvCacheExtParam::tensor_num_per_layer) - 1<br>Currently, tensor_num_per_layer supports a value range of [1, total number of tensors in the current cache], with a default value of 2. When src_layer_range or dst_layer_range uses a non-default value, tensor_num_per_layer can keep the default value or be set to another value. If another value is set, tensor_num_per_layer must also be divisible by the total number of tensors in the current cache. |

## Examples

```cpp
CacheDesc kv_desc{};
kv_desc.data_type = llm_datadist::DT_INT32;
kv_desc.shape = {4, 16};
kv_desc.num_tensors = 4;
Cache cache{};
llm_datadist.AllocateCache(kv_desc, cache);
CacheIndex dst_cache_key{0, 1};
KvCacheExtParam ext_param{};
ext_param.src_layer_range =  std::pair<int32_t, int32_t>(3, 3);
ext_param.dst_layer_range =  std::pair<int32_t, int32_t>(3, 3);
ext_param.tensor_num_per_layer = 1;
std::vector<uint64_t> prompt_blocks = {0, 1, 2, 3};
std::vector<uint64_t> decoder_blocks = {3, 2, 1, 0};
Status ret = llm_datadist.PushKvBlocks(cache, dst_cache_key, prompt_blocks, decoder_blocks , ext_param);
```

## Return Value

- LLM\_SUCCESS: Success
- LLM\_PARAM\_INVALID: Invalid parameter
- LLM\_NOT\_YET\_LINK: No link established with the remote cluster
- LLM\_TIMEOUT: Push timeout
- LLM\_KV\_CACHE\_NOT\_EXIST: The local or remote KV Cache does not exist
- Others: failure

## Constraints

Before calling this API, call [Initialize](Initialize.md) to complete initialization. src\_cache The cache must be the one applied for by the [AllocateCache](AllocateCache.md) API.
