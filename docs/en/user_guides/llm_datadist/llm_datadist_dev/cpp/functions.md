# Feature Overview

<!-- md-trans-meta sourceCommit=c239275db9361d5c1078e1b46ba8934372340ebc translatedAt=2026-11-17T04:07:16.050Z pushedAt=2026-11-17T12:13:56.429Z nl=2 -->

## Link Management

### Link Establishment

**Feature Overview**

Call the LinkLlmClusters API to establish a communication link between PD nodes. The link is mainly used for KV Cache transmission.

**Scenarios**

Link establishment is a prerequisite for data transmission between nodes. In D2D mode, the link establishment API adopts a TCP-like link establishment process. The P side provides listening information at initialization, and the D side initiates link establishment.

Based on the service load, when the PD node ratio of a cluster needs to be adjusted, nodes are scaled out through link establishment.

**Feature Example**

1. Initialize LLM-DataDist. The P side needs to set the Device IP and port for listening.

    ```python
    // P side
    LlmDataDist llm_data_dist(PROMPT_CLUSTER_ID, LlmRole::kPrompt);
    std::map<AscendString, AscendString> options;
    options[OPTION_DEVICE_ID] = "0";
    // Replace with the real IP and port
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
    // Replace with the listening port at P-side initialization
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

When the LinkLlmClusters call fails, check whether the network between the two machines is connected, whether the Device IP is correct, and whether the port is occupied.

For more exception handling, see the error codes.

### Link Disconnection

**Feature Overview**

Call the UnlinkLlmClusters API to disconnect and clean up the communication link between PD nodes.

**Scenarios**

When an exception occurs on a P or D cluster node, the abnormal link is cleaned up through link teardown. Alternatively, when the PD node ratio of a cluster needs to be adjusted, the established link is closed through link teardown.

**Feature Example**

There are two methods for calling link teardown.

One is to initiate link teardown on the D side, which is commonly used in non-fault link scenarios.

```python
// clusters are the same as those used for link establishment
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

The other is to initiate forced link teardown on both the P and D sides, which is commonly used in link fault scenarios.

```python
// Both P and D sides
std::vector<Status> rets;
std::vector<ClusterInfo> clusters;
ClusterInfo cluster_info;
IpInfo local_ip_info;
// Replace with the local real IP
local_ip_info.ip = "ip";
IpInfo remote_ip_info;
// Replace with the peer real IP
remote_ip_info.ip = "ip";
// Replace with the listening port at P-side initialization
remote_ip_info.port = "port";
cluster_info.remote_cluster_id = PROMPT_CLUSTER_ID;
cluster_info.local_ip_infos.emplace_back(std::move(local_ip_info)); // The IPs in local_ip_infos are the local Device IP addresses
cluster_info.remote_ip_infos.emplace_back(std::move(remote_ip_info)); // The IPs in remote_ip_infos are the peer Device IP addresses
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

When the UnlinkLlmClusters call fails, if it is a network fault scenario, the forced link teardown method can be used instead.

For more exception handling, see the error codes.

## KV Cache Management

**Feature Overview**

At LLM-DataDist initialization, a buffer pool of a specified size is pre-applied. Its size is determined by the OPTION\_BUF\_POOL\_CFG configuration item. Subsequent KV Cache buffer allocation and release are performed on the pre-applied buffer, which saves time compared with applying for a block of cache buffer for each execution.

The main APIs and functions involved in KV Cache management are as follows:

|Interface Name|Feature|
|--|--|
|AllocateCache|Allocates a Cache.|
|DeallocateCache|Releases a Cache.|
|PullKvCache|Pulls a Cache from a remote node to the local Cache. This API can be called only when the role is Decoder.|
|PullKvBlocks|In the Blocks Cache transfer scenario, pulls a Cache from a remote node to the local Cache by configuring a block list. This API can be called only when the role is Decoder.|
|CopyKvCache|Copies the KV Cache. Supports D2D and D2H copies. When PullKvCache and other Cache-using operations are expected to be pipelined, an additional transit Cache can be applied for. When other processes are using the Cache, the next Cache can be pulled to the transit Cache first. After the other processes finish using the Cache, it is copied to the specified location, thereby hiding the PullKvCache time consumption through pipelining and reducing the total time consumption. In the common prefix scenario, before inference of a new request, the common prefix can be copied to new buffer and merged with the KV of the current request for inference.|
|CopyKvBlocks|In the PA scenario, copies the KV Cache by using a block list. Supports D2D, D2H, and H2D copies. The D2D scenario mainly targets the case where multiple responses need to share the same block and the block is not full, so newly added tokens need to be copied to a new block to continue iteration. H2D and D2H copies are mainly used for swapping Cache buffer in and out on the corresponding block_index.|
|PushKvCache|Pushes a Cache from the local node to a remote node. This API can be called only when the role is Prompt.|
|PushKvBlocks|In the Blocks Cache transfer scenario, pushes a Cache from the local node to a remote node by configuring a block list. This API can be called only when the role is Prompt.|

**Scenarios**

It is mainly used for KV Cache transmission and movement between distributed clusters.

**Function Example (General Cache Transmission Scenario)**

This example describes the use of APIs in the general cache transmission scenario, mainly involving the allocation, release, and transmission of the KV cache. Pseudo-code examples are provided below based on service roles.

1. The P side and the D side complete the initialization and link establishment of LLM-DataDist according to the example in the [Link Establishment](functions.md#link-establishment) section.
2. On the P/D side, apply for a KV Cache buffer of the corresponding size for each request. If the application fails, release the corresponding resources.

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

3. Transfer the Cache from the P side to the D side in either of the following two ways:
    - Call the PullKvCache API on the D side to pull the KV Cache.

        ```python
        // The P side performs full inference and writes the cache, and notifies the D side that it can pull
        // The D side pulls the cache
        CacheIndex cache_index{PROMPT_CLUSTER_ID, 1, 0};
        ret = llm_data_dist.PullKvCache(cache_index, cache, 0);
        if (ret != LLM_SUCCESS) {
            printf("[ERROR] PullKvCache failed, ret = %u\n", ret);
            return -1;
        }
        // Perform incremental inference
        ```

    - Call the PushKvCache API on the P side to push the KV Cache.

        ```python
        // After the P side completes computation at one layer, it can immediately push in the transmission thread to overlap most of the transmission time with computation.
        CacheIndex dst_cache_index{DECODE_CLUSTER_ID, 1, 0};
        KvCacheExtParam ext_param{};
        ext_param.src_layer_range =  std::pair<int32_t, int32_t>(0, 0);
        ext_param.dst_layer_range =  std::pair<int32_t, int32_t>(0, 0);
        // Number of tensors per layer, which can be modified based on the actual model
        ext_param.tensor_num_per_layer = 2;
        ret = llm_data_dist.PushKvCache(cache, dst_cache_index, 0, -1, ext_param);
        if (ret != LLM_SUCCESS) {
            printf("[ERROR] PushKvCache failed, ret = %u\n", ret);
            return -1;
        }
        ```

4. The P/D side releases the KV Cache buffer of the corresponding request based on the timing of Cache usage in the service.

    ```python
    ret = llm_data_dist.DeallocateCache(cache.cache_id);
    if (ret != LLM_SUCCESS) {
        printf("[ERROR] DeallocateCache failed, ret = %u\n", ret);
    } else {
        printf("[INFO] DeallocateCache success\n");
    }
    ```

5. When the service exits, the P side and the D side perform link teardown according to the example in the [Link Disconnection](functions.md#link-disconnection) section and call the finalize API to release resources.

**Function Example (Blocks Cache Transmission Scenario)**

This example describes how to use the APIs in the Blocks Cache (managing the cache in block form) transmission scenario, mainly involving the allocation, release, and transmission of KV Cache. Pseudo-code examples are provided below based on service roles.

1. The P side and the D side complete the initialization and link establishment of LLM-DataDist according to the example in [Link Establishment](functions.md#link-establishment).
2. On the P side and the D side, call the AllocateCache API to apply for the KV Cache for each layer of the model according to the calculated num\_block quantity. The upper-layer framework reuses the created num\_block-sized KV Cache for different requests, and releases the applied buffer after the service ends.

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

3. After the P side receives a new request, it allocates the corresponding block\_index for each request. This is a feature provided by the inference framework. After model inference is complete, the KV Cache corresponding to the request resides in the buffer where the corresponding block\_index is located. The model output and the block\_table corresponding to the request are then transmitted to the D-side inference model as input.
4. After the D side receives a new request, it allocates the corresponding block\_index_ for each request, and then calls the pull\_blocks API to transmit the KV Cache to the specified location based on the correspondence between the P-side block\_index and the D-side block\_index.
    - Call the PullKvBlocks API on the D side to pull the KV Cache.

        ```python
        // The P side performs full inference and writes the cache, and notifies the D side that it can pull the cache
        // The D side pulls the cache
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
        // After the P side completes computation at one layer, it can immediately push in the transmission thread to overlap most of the transmission time with computation.
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

5. The P/D side releases the KV Cache buffer of the corresponding request based on the timing of Cache usage in the service.

    ```python
    ret = llm_data_dist.DeallocateCache(cache.cache_id);
    if (ret != LLM_SUCCESS) {
        printf("[ERROR] DeallocateCache failed, ret = %u\n", ret);
    } else {
        printf("[INFO] DeallocateCache success\n");
    }
    ```

6. When the service exits, the P side and the D side perform link teardown and resource release according to the example in the [Link Disconnection](functions.md#link-disconnection) section.

**Exception Handling**

- Error code LLM\_DEVICE\_OUT\_OF\_MEMORY indicates that the Device failed to allocate the KV Cache buffer. Check the OPTION\_BUF\_POOL\_CFG value set at initialization and the size of the allocated KV Cache, and verify whether any requested KV Cache has not been released after being pulled.
- Error code LLM\_KV\_CACHE\_NOT\_EXIST indicates that the peer KV Cache does not exist. Check whether the peer process is abnormal or whether the inference of the request corresponding to the KV Cache is complete. This error does not affect the process of other requests. After confirming the process, retry is allowed.
- Error code LLM\_WAIT\_PROCESS\_TIMEOUT or LLM\_TIMEOUT indicates a pull KV timeout, which means a problem has occurred on the link. Retry link teardown or link establishment.
- Error code LLM\_NOT\_YET\_LINK indicates that no link has been established with the remote cluster.
