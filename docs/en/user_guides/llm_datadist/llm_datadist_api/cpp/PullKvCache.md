# PullKvCache

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:22:30.553Z pushedAt=2026-10-29T03:25:50.748Z nl=2 -->

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

Pulls Cache from a remote node to the local Cache. This API can be called only when the role is Decoder.

## Prototype

```cpp
Status PullKvCache(const CacheIndex &src_cache_index,
                   const Cache &dst_cache,
                   uint32_t batch_index = 0U,
                   int64_t size = -1,
                   const KvCacheExtParam &ext_param = {})
```

## Parameters

| Parameter | Input/Output | Value Description |
| --- | --- | --- |
| src_cache_index | Input | Index of the remote source Cache. |
| dst_cache | Input | Local destination Cache. |
| batch_index | Input | Index of the local destination batch. |
| size | Input | Set to an integer greater than 0, indicating the size to pull.<br>Or set to -1, indicating a full pull.<br>The default value is -1. |
| ext_param | Input | Currently, the difference between second and first of src_layer_range in ext_param must be consistent with the difference between second and first of dst_layer_range. The default values of first and second of both src_layer_range and dst_layer_range are -1, indicating all layers. The value range is [0, maximum available layer index], and first must be less than or equal to second. The maximum available layer index is calculated as follows.<br>(CacheDesc::num_tensors / KvCacheExtParam::tensor_num_per_layer) - 1<br>Currently, tensor_num_per_layer supports a value range of [1, total number of tensors in the current cache], with a default value of 2. When src_layer_range or dst_layer_range is set to a non-default value, tensor_num_per_layer can keep the default value or be set to another value. If another value is set, tensor_num_per_layer must be divisible by the total number of tensors in the current cache. |

## Examples

```cpp
CacheIndex cache_index;
cache_index.cluster_id = 0;
cache_index.cache_id = cached_tensors.cache_id;
cache_index.batch_index = 0;
Status ret = llm_datadist.PullKvCache(cache_index, cache);
```

## Return Value

- LLM\_SUCCESS: Success
- LLM\_PARAM\_INVALID: Invalid parameter
- LLM\_NOT\_YET\_LINK: No link established with the remote cluster
- LLM\_TIMEOUT: Pull timeout
- LLM\_KV\_CACHE\_NOT\_EXIST: The local or remote KV Cache does not exist
- Others: Failure

## Constraints

Before calling this API, call [Initialize](Initialize.md) to complete initialization. dst\_cache The cache must be the one allocated by the [AllocateCache](AllocateCache.md) API.
