# PullKvBlocks

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:22:31.506Z pushedAt=2026-10-29T03:25:50.747Z nl=2 -->

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

In the Blocks Cache transfer scenario, pulls the Cache from a remote node to the local Cache by using a block list. This API can be called only when the role is Decoder.

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
| src_cache_index | Input | Index of the source Cache on the remote node. |
| dst_cache | Input | Local destination Cache. |
| src_blocks | Input | Block index list of the source Cache. |
| dst_blocks | Input | Block index list of the destination Cache. |
| ext_param | Input | Currently, the difference between the second and first values of src_layer_range in ext_param must be the same as the difference between the second and first values of dst_layer_range. The default values of first and second in both src_layer_range and dst_layer_range are -1, which indicates all layers. The value range is [0, maximum available layer index], and first must be less than or equal to second. The maximum available layer index is calculated as follows.<br>(CacheDesc::num_tensors / KvCacheExtParam::tensor_num_per_layer) - 1<br>Currently, tensor_num_per_layer supports a value range of [1, total number of tensors in the current cache], with a default value of 2. When src_layer_range or dst_layer_range uses a non-default value, tensor_num_per_layer can keep the default value or use another value. If another value is used, tensor_num_per_layer must be divisible by the total number of tensors in the current cache. |

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
- Others: failure

## Constraints

Before calling this API, call [Initialize](Initialize.md) to complete initialization. dst\_cache must be a Cache that has been allocated by the [AllocateCache](AllocateCache.md) API.
