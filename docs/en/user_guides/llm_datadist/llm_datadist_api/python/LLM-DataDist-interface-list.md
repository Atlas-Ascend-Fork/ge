# LLM-DataDist-interface-list

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:41:12.477Z pushedAt=2026-10-29T06:18:05.723Z nl=2 -->

LLM-DataDist is a large model distributed cluster and data acceleration component that provides cluster KV data management capabilities to support the separate deployment of full graphs and incremental graphs.

- Supported product forms are as follows:
  - Atlas A2 inference products
  - Atlas A3 products

- Currently, only Python 3.9 and Python 3.11 are supported. For installation methods, see the official Python website [https://www.python.org/](https://www.python.org/).
- A maximum of 50 GB of Device memory can be registered. The larger the registered memory, the more OS memory is occupied.

The LLM-DataDist interface list is as follows.

## LLM-DataDist

**Table 1** LLM-DataDist interfaces

| Interface Name | Overview |
| --- | --- |
| [LLMDataDist-constructor](LLMDataDist-constructor.md) | Constructs an LLMDataDist instance. |
| [init](init.md) | Initializes LLMDataDist. |
| [finalize](finalize.md) | Releases LLMDataDist. |
| [link_clusters](link_clusters.md) | Establishes links. |
| [unlink_clusters](unlink_clusters.md) | Disconnects links. |
| [check_link_status](check_link_status.md) | Quickly checks whether the link status is normal. |
| [kv_cache_manager](kv_cache_manager.md) | Obtains a KvCacheManager instance. |
| [switch_role](switch_role.md) | Switches the role of the current LLMDataDist. It is recommended to use this API only in PagedAttention scenarios. |

## LLMConfig

**Table 2** LLMConfig APIs

| Interface Name | Overview |
| --- | --- |
| [LLMConfig-constructor](LLMConfig-constructor.md) | Constructs an LLMConfig. |
| [generate_options](generate_options.md) | Generates a dictionary of configuration items. |
| [device_id](device_id.md) | Sets the device ID of the current process, corresponding to the underlying ge.exec.deviceId configuration item. |
| [sync_kv_timeout](sync_kv_timeout.md) | Configures the timeout for APIs such as pulling kv, corresponding to the underlying llm.SyncKvCacheWaitTime configuration item. |
| [enable_switch_role](enable_switch_role.md) | Configures whether smooth role switching is supported, corresponding to the underlying llm.EnableSwitchRole configuration item. |
| [ge_options](ge_options.md) | Configures additional GE configuration items. |
| [listen_ip_info](listen_ip_info.md) | Sets the cluster listening information on the PROMPT side, corresponding to the underlying llm.listenIpInfo configuration item. |
| [mem_utilization](mem_utilization.md) | Configures the utilization of the ge.flowGraphMemMaxSize memory. The default value is 0.95. |
| [buf_pool_cfg](buf_pool_cfg.md) | Specifies the memory tier configuration to improve memory application performance and utilization. |

## KvCacheManager

**Table 3** KvCacheManager APIs

| Interface Name | Overview |
| --- | --- |
| [KvCacheManager-constructor](KvCacheManager-constructor.md) | Introduces KvCacheManager-constructor. |
| [is_initialized](is_initialized.md) | Queries whether the KvCacheManager instance has been initialized. |
| [allocate_cache](allocate_cache.md) | Allocates a cache. After the cache is allocated successfully, it is referenced by both cache_id and cache_keys. The resources occupied by the cache are actually released only after all these references are removed. |
| [deallocate_cache](deallocate_cache.md) | Releases a cache. |
| [remove_cache_key](remove_cache_key.md) | Removes a CacheKey. This API can be called only when LLMRole is PROMPT. |
| [pull_cache](pull_cache.md) | Pulls KV from the corresponding Prompt node to the local KV Cache based on the CacheKey. This API can be called only when LLMRole is DECODER. |
| [copy_cache](copy_cache.md) | Copies KV. |
| [get_cache_tensors](get_cache_tensors.md) | Obtains cache tensors. |
| [allocate_blocks_cache](allocate_blocks_cache.md) | Allocates a cache of multiple blocks in the PagedAttention scenario. |
| [pull_blocks](pull_blocks.md) | In the PagedAttention scenario, pulls KV from the corresponding Prompt node to the local KV Cache in the form of a block list based on the BlocksCacheKey. This API can be called only when LLMRole is DECODER. |
| [copy_blocks](copy_blocks.md) | Copies KV in the PagedAttention scenario. |
| [swap_blocks](swap_blocks.md) | Swaps data in and out between cpu_cache and npu_cache. |
| [transfer_cache_async](transfer_cache_async.md) | Transfers KV Cache asynchronously in a hierarchical manner. |

## KvCache

**Table 4** KVCache APIs

| Interface Name | Overview |
| --- | --- |
| [KvCache-constructor](KvCache-constructor.md) | Constructs KVCache. |
| [cache_id](cache_id.md) | Obtains the ID of KvCache. |
| [cache_desc](cache_desc.md) | Obtains the description of KvCache. |
| [per_device_tensor_addrs](per_device_tensor_addrs.md) | Obtains the address of KvCache. |
| [create_cpu_cache](create_cpu_cache.md) | Creates a CPU cache. |

## LLMClusterInfo

**Table 5** LLMClusterInfo APIs

| Interface Name | Overview |
| --- | --- |
| [LLMClusterInfo-constructor](LLMClusterInfo-constructor.md) | Constructs LLMClusterInfo. |
| [remote_cluster_id](remote_cluster_id.md) | Sets the peer cluster ID. |
| [append_local_ip_info](append_local_ip_info.md) | Adds local cluster IP information. |
| [append_remote_ip_info](append_remote_ip_info.md) | Adds remote cluster IP information. |

## CacheTask

**Table 6**  CacheTask

| Interface Name | Overview |
| --- | --- |
| [CacheTask-constructor](CacheTask-constructor.md) | Constructs a CacheTask. |
| [synchronize](synchronize.md) | Waits for all layers to complete transfer and obtains the overall execution result. |
| [get_results](get_results.md) | Waits for all layers to complete transfer and obtains the execution result corresponding to each TransferConfig. |

## Others

**Table 7** Others

| Interface Name | Overview |
| --- | --- |
| [LLMRole](LLMRole.md) | Enumeration values of LLMRole. |
| [Placement](Placement.md) | A field of CacheDesc, indicating the device type where the cache resides. |
| [CacheDesc](CacheDesc.md) | Constructs CacheDesc. |
| [CacheKey](CacheKey.md) | Constructs CacheKey. |
| [CacheKeyByIdAndIndex](CacheKeyByIdAndIndex.md) | Constructs CacheKeyByIdAndIndex, which is typically used as a parameter type in the [pull_cache](pull_cache.md) API. |
| [BlocksCacheKey](BlocksCacheKey.md) | Constructs BlocksCacheKey in the PagedAttention scenario. |
| [LayerSynchronizer](LayerSynchronizer.md) | Waits for the specified layer of the model to finish execution. Users need to inherit LayerSynchronizer and implement this API.<br>This API is called when KvCacheManager.transfer_cache_async is executed. When this API returns success, the transfer of the current layer's cache starts. |
| [TransferConfig](TransferConfig.md) | Constructs TransferConfig. |
| [TransferWithCacheKeyConfig](TransferWithCacheKeyConfig.md) | Constructs TransferWithCacheKeyConfig. |
| [LLMException](LLMException.md) | Obtains the error-code of the exception. For the error-code list, see [LLMStatusCode](LLMStatusCode.md). |
| [LLMStatusCode](LLMStatusCode.md) | Enumeration values of LLMStatusCode. |
| [DataType](DataType.md) | Enumeration class of DataType. |
