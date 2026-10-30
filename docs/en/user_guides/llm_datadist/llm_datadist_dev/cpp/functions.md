# Feature Overview

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-10-29T03:41:15.644Z pushedAt=2026-10-29T07:13:44.139Z nl=2 -->

## Link Management

### Link Establishment

**Feature Overview**

Call the LinkLlmClusters interface to establish a communication link between PD nodes. The link is mainly used for KV Cache transmission.

**Use Scenario**

Link establishment is a prerequisite for data transmission between nodes. In D2D mode, the link establishment interface adopts a TCP-like link establishment process. The P side provides listening information during initialization, and the D side initiates link establishment.

Based on service load, when the PD node ratio of the cluster needs to be adjusted, nodes can be scaled out through link establishment.

**Function Example**

1. Initialize LLM-DataDist. The P side needs to set the listening Device IP and port.

    ```python
    // P side
    LlmDataDist llm_data_dist(PROMPT_CLUSTER_ID, LlmRole::kPrompt);
    std::map<AscendString, AscendString> options;
    options[OPTION_DEVICE_ID] = "0";
    // Replace with the actual IP and port.
    options[OPTION_LISTEN_IP_INFO] = "ip:port";
    options[OPTION_BUF_POOL_CFG] = R"({
    "buf_cfg":[{"total_size":2097152,"blk_size":256,"max_buf_size":8192}],
    "buf_pool_size": 2147483648
    })";
    auto ret = llm_data_dist.Initialize(options);
    if (ret != LLM_SUCCESS) {
        printf("[ERROR] Initialize failed, ret = %u\n", ret);
        return -1;
    }
    // D side
    LlmDataDist llm_data_dist(DECODER_CLUSTER_ID, LlmRole::kDecoder);
    std::map<AscendString, AscendString> options;
    options[OPTION_DEVICE_ID] = "0";
    options[OPTION_BUF_POOL_CFG] = R"({
    "buf_pool_size": 2147483648
    })";
    auto ret = llm_data_dist.Initialize(options);
    if (ret != LLM_SUCCESS) {
        printf("[ERROR] Initialize failed, ret = %u\n", ret);
        return -1;
    }
    ```

2. Call the LinkLlmClusters API on the D side to initiate link establishment.

    ```python
    std::vector<Status> rets;
    std::vector<ClusterInfo> clusters;
    ClusterInfo cluster_info;
    IpInfo local_ip_info;
    // Replace with the local real IP
    local_ip_info.ip = "ip";
    IpInfo remote_ip_info;
    // Replace with the peer real IP
    remote_ip_info.ip = "ip";
    // Replace with the listening port used during P-side initialization
    remote_ip_info.port = "port";
    cluster_info.remote_cluster_id = PROMPT_CLUSTER_ID;
    cluster_info.local_ip_infos.emplace_back(std::move(local_ip_info));
    cluster_info.remote_ip_infos.emplace_back(std::move(remote_ip_info));
    clusters.emplace_back(std::move(cluster_info));
    auto ret = llm_data_dist.LinkLlmClusters(clusters, rets);
    if (ret != LLM_SUCCESS) {
        printf("[ERROR] LinkLlmClusters failed, ret = %u\n", ret);
        return -1;
    }
    for (const auto &inner_ret : rets) {
        if (inner_ret != LLM_SUCCESS) {
            printf("[ERROR] LinkLlmClusters failed, ret = %u\n", inner_ret);
            return -1;
        }
    }
    ```

**Exception Handling**

If the LinkLlmClusters call fails, check whether the networks of the two machines are connected, whether the device IP is correct, and whether the port is occupied.

For more details about exception handling, see the error codes.

### Link Disconnection

**Feature Overview**

Call the UnlinkLlmClusters interface to disconnect and clean up the communication link between PDs.

**Use Scenario**

When a P or D cluster node is abnormal, unlink to clean up the abnormal link. Alternatively, when the PD node ratio of the cluster needs to be adjusted, unlink to close the established link.

**Function Example**

There are two ways to call the unlink operation.

One is to initiate the unlink on the D side, which is commonly used in non-fault link scenarios.

```python
// clusters are the same as those used for link establishment.
auto ret = llm_data_dist.UnlinkLlmClusters(clusters, rets);
if (ret != LLM_SUCCESS) {
    printf("[ERROR] UnlinkLlmClusters failed, ret = %u\n", ret);
    return -1;
}
for (const auto &inner_ret : rets) {
    if (inner_ret != LLM_SUCCESS) {
        printf("[ERROR] UnlinkLlmClusters failed, ret = %u\n", inner_ret);
        return -1;
    }
}
```

The other is to initiate a forced unlink on both PD sides, which is commonly used in link fault scenarios.

```python
// Both PD sides.
std::vector<Status> rets;
std::vector<ClusterInfo> clusters;
ClusterInfo cluster_info;
IpInfo local_ip_info;
// Replace with the local real IP address
local_ip_info.ip = "ip";
IpInfo remote_ip_info;
// Replace with the peer real IP address
remote_ip_info.ip = "ip";
// Replace with the listening port used during initialization on the P side
remote_ip_info.port = "port";
cluster_info.remote_cluster_id = PROMPT_CLUSTER_ID;
cluster_info.local_ip_infos.emplace_back(std::move(local_ip_info)); // The IP in local_ip_infos is the local Device IP address
cluster_info.remote_ip_infos.emplace_back(std::move(remote_ip_info)); // The IP in remote_ip_infos is the peer Device IP address
clusters.emplace_back(std::move(cluster_info));
auto ret = llmDataDist.UnlinkLlmClusters(clusters, rets, 1000, true);
if (ret != LLM_SUCCESS) {
    printf("[ERROR] UnlinkLlmClusters failed, ret = %u\n", ret);
    return -1;
}
for (const auto &inner_ret : rets) {
    if (inner_ret != LLM_SUCCESS) {
        printf("[ERROR] UnlinkLlmClusters failed, ret = %u\n", inner_ret);
        return -1;
    }
}
```

**Exception Handling**

If UnlinkLlmClusters fails to be called, in a network fault scenario, you can use the forced unlink method instead.

For more exception handling, see Error Code.

## KV Cache Management

**Feature Overview**

During LLM-DataDist initialization, a memory pool of a specified size is pre-allocated. Its size is determined by the OPTION\_BUF\_POOL\_CFG configuration item. Subsequent KV Cache memory allocation and release are performed on the pre-allocated memory, which saves time compared with allocating a block of cache memory for each execution.

The main APIs involved in KV Cache management and their functions are as follows:

|Interface Name|Description|
|--|--|
|AllocateCache|Allocates a cache.|
|DeallocateCache|Releases a cache.|
|PullKvCache|Pulls Cache from a remote node to the local Cache. This API can be called only when the role is Decoder.|
|PullKvBlocks|In the Blocks Cache transfer scenario, pulls the cache from a remote node to the local cache by configuring a block list. This API can be called only when the role is Decoder.|
|CopyKvCache|Copies the KV Cache. D2D and D2H copies are supported. When PullKvCache and other cache-consuming operations are expected to be pipelined, an additional transit cache can be allocated. When other processes are using the cache, the next cache can be pulled to the transit cache first. After the other processes finish using the cache, the cache is copied to the specified location, thereby hiding the PullKvCache latency through pipelining and reducing the total latency. In the common prefix scenario, before inference of a new request, the common prefix can be copied to new memory and merged with the KV of the current request for inference.|
|CopyKvBlocks|In the PA scenario, copies the KV Cache by using a block list. D2D, D2H, and H2D copies are supported. The D2D scenario is mainly for the case where multiple responses need to share the same block and the block is not full, so newly added tokens need to be copied to a new block to continue iteration. H2D and D2H copies are mainly used for swapping cache memory in and out on the corresponding block_index.|
|PushKvCache|Pushes the cache from the local node to a remote node. This API can be called only when the role is Prompt.|
|PushKvBlocks|In the Blocks Cache transfer scenario, pushes the cache from the local node to a remote node by configuring a block list. This API can be called only when the role is Prompt.|

**Use Scenarios**

It is mainly used for KV Cache transfer and movement between nodes in a distributed cluster.

**Function Example (General Cache Transfer Scenario)**

This example describes the use of the APIs in the general cache transfer scenario, mainly involving the allocation, release, and transfer of the KV Cache. The following provides pseudocode examples based on the service roles.

1. On the P side and D side, complete the initialization and link establishment of LLM-DataDist according to the example in the [Link Establishment](functions.md#link-establishment) section.
2. On the P/D side, allocate KV Cache memory of the corresponding size for each request. If the allocation fails, release the corresponding resources.

    ```python
    void OnError(LlmDataDist &llm_data_dist, Cache &cache)
    {
        if (cache.cache_id > 0) {
            (void) llmDataDist.DeallocateCache(cache.cache_id);
        }
        llm_data_dist.Finalize();
    }
    CacheDesc kv_cache_desc{};
    kv_cache_desc.num_tensors = NUM_TENSORS;
    kv_cache_desc.data_type = DT_INT32;
    kv_cache_desc.shape = {8, 16};
    Cache cache{};
    auto ret = llm_data_dist.AllocateCache(kv_cache_desc, cache);
    if (ret != LLM_SUCCESS) {
        printf("[ERROR] AllocateCache failed, ret = %u\n", ret);
        OnError(llm_data_dist, cache);
        return -1;
    }
    ```

3. Transfer the cache from the P side to the D side in either of the following two ways:
    - Call the PullKvCache API on the D side to pull the KV cache.

        ```python
        // The P side performs full inference to write the cache and notifies the D side that it can pull.
        // The D side pulls the cache.
        CacheIndex cache_index{PROMPT_CLUSTER_ID, 1, 0};
        ret = llm_data_dist.PullKvCache(cache_index, cache, 0);
        if (ret != LLM_SUCCESS) {
            printf("[ERROR] PullKvCache failed, ret = %u\n", ret);
            return -1;
        }
        // Perform incremental inference.
        ```

    - Call the PushKvCache API on the P side to push the KV cache.

        ```python
        // After computing one layer, the P side can push immediately in the transfer thread to overlap most of the transfer time with computation.
        CacheIndex dst_cache_index{DECODE_CLUSTER_ID, 1, 0};
        KvCacheExtParam ext_param{};
        ext_param.src_layer_range =  std::pair<int32_t, int32_t>(0, 0);
        ext_param.dst_layer_range =  std::pair<int32_t, int32_t>(0, 0);
        // Number of tensors per layer, which can be modified based on the actual model.
        ext_param.tensor_num_per_layer = 2;
        ret = llm_data_dist.PushKvCache(cache, dst_cache_index, 0, -1, ext_param);
        if (ret != LLM_SUCCESS) {
            printf("[ERROR] PushKvCache failed, ret = %u\n", ret);
            return -1;
        }
        ```

4. The P/D side releases the KV cache memory of the corresponding request based on the timing of cache usage in the service.

    ```python
    ret = llm_data_dist.DeallocateCache(cache.cache_id);
    if (ret != LLM_SUCCESS) {
        printf("[ERROR] DeallocateCache failed, ret = %u\n", ret);
    } else {
        printf("[INFO] DeallocateCache success\n");
    }
    ```

5. When the service exits, the P side and D side disconnect the link and call the finalize API to release resources as shown in the example in the [Link Disconnection](functions.md#link-disconnection) section.

**Function Example (Blocks Cache Transmission Scenario)**

This example describes how to use the APIs in the Blocks Cache transmission scenario, in which the cache is managed in block form. It mainly involves the allocation, release, and transmission of the KV cache. The following provides pseudo-code examples based on business roles.

1. The P side and D side complete the initialization and link establishment of LLM-DataDist based on the example in [Link Establishment](functions.md#link-establishment).
2. On each layer of the P-side and D-side models, call the AllocateCache API to allocate the KV cache based on the calculated number of num\_block. The upper-layer framework reuses the created KV cache of num\_block size for different requests, and releases the allocated memory after the service ends.

    ```python
    void OnError(LlmDataDist &llm_data_dist, Cache &cache)
    {
        if (cache.cache_id > 0) {
            (void) llm_data_dist.DeallocateCache(cache.cache_id);
        }
        llm_data_dist.Finalize();
    }
    CacheDesc kv_cache_desc{};
    kv_cache_desc.num_tensors = NUM_TENSORS;
    kv_cache_desc.data_type = DT_INT32;
    kv_cache_desc.shape = {8, 16};
    Cache cache{};
    auto ret = llm_data_dist.AllocateCache(kv_cache_desc, cache);
    if (ret != LLM_SUCCESS) {
        printf("[ERROR] AllocateCache failed, ret = %u\n", ret);
        OnError(llm_data_dist, cache);
        return -1;
    }
    ```

3. After receiving a new request, the P side allocates the corresponding block\_index for each request. This is a function provided by the inference framework. After model inference is complete, the KV cache corresponding to the request resides in the memory where the corresponding block\_index is located. The model output and the block\_table corresponding to the request are then transmitted to the D-side inference model as input.
4. After receiving a new request, the D side allocates the corresponding block\_index_ for each request, _and then calls the pull\_blocks API to transmit the KV cache to the specified location based on the correspondence between the block\_index on the P side and the block\_index on the D side.
    - Call the PullKvBlocks API on the D side to pull the KV cache.

        ```python
        // Perform full inference on the P side to write the cache, and notify the D side that the cache can be pulled
        // Pull the cache on the D side
        CacheIndex cache_index{PROMPT_CLUSTER_ID, 1, 0};
        std::vector<uint64_t> prompt_blocks = {0, 1, 2, 3};
        std::vector<uint64_t> decoder_blocks = {3, 2, 1, 0};
        auto ret = llm_data_dist.PullKvBlocks(cache_index, cache, prompt_blocks, decoder_blocks);
        if (ret != LLM_SUCCESS) {
            printf("[ERROR] PullKvBlocks failed, ret = %u\n", ret);
            return -1;
        }
        // Perform incremental inference
        ```

    - Call the PushKvBlocks API on the P side to push the KV Cache.

        ```python
        // After computing one layer, the P side can push it immediately in the transmission thread to overlap most of the transmission time with computation.
        CacheIndex dst_cache_index{DECODE_CLUSTER_ID, 1};
        KvCacheExtParam ext_param{};
        ext_param.src_layer_range =  std::pair<int32_t, int32_t>(0, 0);
        ext_param.dst_layer_range =  std::pair<int32_t, int32_t>(0, 0);
        // Number of tensors per layer, which can be modified based on the actual model
        ext_param.tensor_num_per_layer = 2;
        std::vector<uint64_t> prompt_blocks = {0, 1, 2, 3};
        std::vector<uint64_t> decoder_blocks = {3, 2, 1, 0};
        ret = llm_data_dist.PushKvBlocks(cache, dst_cache_index, prompt_blocks, decoder_blocks, ext_param);
        if (ret != LLM_SUCCESS) {
            printf("[ERROR] PushKvBlocks failed, ret = %u\n", ret);
            return -1;
        }
        ```

5. The P/D side releases the KV Cache memory of the corresponding request based on the timing of Cache usage in the service.

    ```python
    ret = llm_data_dist.DeallocateCache(cache.cache_id);
    if (ret != LLM_SUCCESS) {
        printf("[ERROR] DeallocateCache failed, ret = %u\n", ret);
    } else {
        printf("[INFO] DeallocateCache success\n");
    }
    ```

6. When the service exits, the P side and D side disconnect and release resources according to the example in the [Link Disconnection](functions.md#link-disconnection) section.

**Exception Handling**

- Error code LLM\_DEVICE\_OUT\_OF\_MEMORY indicates that the Device failed to apply for KV Cache memory. Check the OPTION\_BUF\_POOL\_CFG size set during initialization and the size of the requested KV Cache, and verify whether any KV Cache memory has not been released after being pulled.
- Error code LLM\_KV\_CACHE\_NOT\_EXIST indicates that the KV Cache on the peer does not exist. Check whether the peer process is abnormal or whether the inference of the request corresponding to the KV Cache has completed. This error does not affect other request processes. You can retry after confirming the process.
- Error code LLM\_WAIT\_PROCESS\_TIMEOUT or LLM\_TIMEOUT indicates a pull KV timeout, meaning that a link problem has occurred. Try disconnecting or re-establishing the link.
- Error code LLM\_NOT\_YET\_LINK indicates that no link has been established with the remote cluster.
