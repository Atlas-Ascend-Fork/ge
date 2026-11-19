# PullKvCache

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:58:56.290Z pushedAt=2026-11-17T06:39:16.613Z nl=2 -->

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

Pulls the Cache from a remote node to the local Cache. This function can be called only when the role is Decoder.

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
| src_cache_index | Input | Index of the remote source cache. |
| dst_cache | Input | Local destination cache. |
| batch_index | Input | Index of the local destination batch. |
| size | Input | The value must be an integer greater than 0, indicating the size to pull.<br>If this parameter is set to **–1**, it indicates a full pull.<br>The default value is **–1**. |
| ext_param | Input | Currently, the difference between **second** and **first** in **src_layer_range** of **ext_param** must be the same as the difference between **second** and **first** in **dst_layer_range**. The default values of **first** and **second** in both **src_layer_range** and **dst_layer_range** are **–1**, indicating all layers. The value range of both is [0, maximum available layer index], and **first** must be less than or equal to **second**. The maximum available layer index is calculated as follows.<br>(CacheDesc::num_tensors / KvCacheExtParam::tensor_num_per_layer) - 1<br>Currently, tensor_num_per_layer supports a value range of [1, total number of tensors in the current cache], with a default value of **2**. When **src_layer_range** or **dst_layer_range** uses a non-default value, **tensor_num_per_layer** can use the default value or be set to another value. If another value is set, **tensor_num_per_layer** must also be divisible by the total number of tensors in the current cache. |

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
- LLM\_PARAM\_INVALID: Parameter error
- LLM\_NOT\_YET\_LINK: No link established with the remote cluster
- LLM\_TIMEOUT: Pull timeout
- LLM\_KV\_CACHE\_NOT\_EXIST: The local or remote KV Cache does not exist
- Others: Failure

## Constraints

Before calling this API, call [Initialize](Initialize.md) to complete initialization. dst\_cache The cache must be the one allocated by the [AllocateCache](AllocateCache.md) API.
