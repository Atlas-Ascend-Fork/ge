# Feature Overview

<!-- md-trans-meta sourceCommit=b2245cacdabb9e821718e669c704062e4ca31a5c translatedAt=2026-10-29T03:46:07.441Z pushedAt=2026-10-29T07:26:39.200Z nl=2 -->

## Link Management

**Feature Overview**

The HCCL collective communication APIs invoked during NN model execution are bilateral, meaning that both sides must initiate link establishment simultaneously. In the P-D separation solution, however, link establishment is simplified so that it is initiated by the Client side alone. Because the dynamically scaled portion is mostly on the Decode side, P is defined as the server and D as the client, and the link establishment process is implemented as a flow in which D initiates link establishment toward P.

Two APIs are mainly provided: link\_clusters and unlink\_clusters. Both are invoked by the D side, and the link establishment behavior is point-to-point.

- link\_clusters is used to establish links between nodes.
- unlink\_clusters is used to unlink nodes.

**Scenarios**

- Link establishment is a prerequisite for KV Cache transmission between P and D. Therefore, links must be established before the KV Cache transmission function is enabled.
- In cluster reliability scenarios, when a node in the P or D cluster becomes faulty, the faulty node is taken offline by unlinking it without affecting the availability of the entire cluster.
- The P-D cluster ratio is dynamically adjusted through link establishment and unlinking. Machine nodes are dynamically added or removed based on their busy or idle status. Adding a node requires link establishment, while removing a node requires unlinking.

**Function Example**

The following code example shows the pseudocode process of link establishment between 1P and 1D.

1. Start the P-side and D-side scripts, in which the initialization API of LLM-DataDist is called. The P side needs to set the device IP and port to listen on.

    ```python
    # P-side script
    from llm_datadist import LLMDataDist, LLMRole, LLMStatusCode, LLMClusterInfo

     # Initialize llm datadist
     llm_datadist = LLMDataDist(LLMRole.Prompt, cluster_id=0)
     llm_config = LLMConfig()
     llm_config.listen_ip_info ="192.168.1.1:26000"# local_ip + port
     llm_config.device_id =0  # The device_id here must match local_ip
     llm_options = llm_config.generate_options()
     llm_datadist.init(llm_options)
    ```

    ```python
    # D-side script
    from llm_datadist import LLMDataDist, LLMRole, LLMStatusCode, LLMClusterInfo

     # Initialize llm datadist
     llm_datadist = LLMDataDist(LLMRole.DECODER, cluster_id=0)
     llm_config = LLMConfig()
     llm_config.device_id =0
     llm_options = llm_config.generate_options()
     llm_datadist.init(llm_options)
    ```

2. In the D-side script, call link\_clusters to initiate link establishment. When the service ends, call unlink\_clusters on the D side to disconnect the link.

    ```python
     # Generate cluster info for link establishment
     cluster = LLMClusterInfo()
     cluster.remote_cluster_id = 1  # The remote_cluster_id here must correspond to the LLMDataDist created on the P side
     cluster.append_local_ip_info("192.168.2.1", 26000) # The IP in local_ip_info is the Device IP address on the local machine for link establishment
     cluster.append_remote_ip_info("192.168.1.1", 26000) # The IP in remote_ip_info is the Device IP address of the peer for link establishment

     # Call link_clusters to establish the link
     # ret is the return value of the API, and rets indicates the link establishment result of each cluster
     ret, rets = llm_datadist.link_clusters([cluster], timeout=5000)
     # Check the link establishment result
     if ret != LLMStatusCode.LLM_SUCCESS:
         raise Exception("link failed.")
     for cluster_i in range(len(rets)):
         link_ret = rets[cluster_i]
         if link_ret != LLMStatusCode.LLM_SUCCESS:
             print(f"{cluster_i} link failed.")
    ```

3. On the D side, you can call check\_link\_status to quickly check whether the link transmission data is normal.

    ```python
    try:
        llm_datadist.check_link_status(remote_cluster_id=1)
    except LLMException as ex:
        print(f"check_link_status exception:{ex.status_code}")
        raise ex
    ```

4. At the service end, the D side unlinks the link, and both the P and D sides call the finalize method of llm\_datadist to release resources.

    ```python
     # P-side script
     # Call llm_datadist to allocate the KV Cache
     # Execute service inference
     # ...

     # Service exit. Call this after the D-side unlinks.
     llm_datadist.finalize()
    ```

    ```python
     # D-side script
     # pull_cache, model inference
     # ...

     # Service exit. Call unlink_clusters to unlink.
     ret, rets = llm_datadist.unlink_clusters([cluster], timeout=5000)
     if ret != LLMStatusCode.LLM_SUCCESS:
         raise RuntimeError(f'[unlink_cluster] failed, ret={ret}')
     llm_datadist.finalize()
    ```

When a new node is added or a previously offline node comes back online, the preceding usage process must be executed again. When a node goes offline, the D-side should proactively call the unlink\_clusters API under normal circumstances. If the D-side cannot call the unlink\_clusters API, the P-side must call the unlink\_clusters API instead.

Dynamic scaling of the distributed cluster is flexibly achieved by calling the link\_clusters API when a node comes online and the unlink\_clusters API when a node goes offline.

**Exception Handling**

When an exception on the D-side prevents it from calling unlink\_clusters, the P-side must call unlink\_clusters to clean up resources. Otherwise, link establishment cannot be performed again.

The unlink\_clusters API provides the capability of forced unlinking, which is applicable when a link fault occurs and a normal unlink operation would take a long time. When the forced unlink API is used (with force=True), both sides must initiate the call, and only the local link is cleaned up.

```python
# Forced unlink
 ret, rets = llm_datadist.unlink_clusters([cluster], timeout=5000, force=True)
 if ret != LLMStatusCode.LLM_SUCCESS:
     raise RuntimeError(f'[unlink_clusters] failed, ret={ret}')
```

## KV Cache Management

**Feature Overview**

During LLM-DataDist initialization, a memory pool of a specified size is pre-allocated. Its size is determined by the ge.flowGraphMemMaxSize configuration item. Subsequent KV cache buffer allocation and release are performed on the pre-allocated memory, which saves time compared with allocating a buffer each time.

The main APIs and functions involved in KV Cache management are as follows:

**Table 1** Main APIs and functions for KV Cache management

|Interface Name|Description|
|--|--|
|allocate_cache|Allocates a cache. After the cache is successfully allocated, it is referenced by both cache_id and cache_keys. The resources occupied by the cache are actually released only after all these references are removed. cache_keys is passed in when LLMRole is PROMPT.|
|deallocate_cache|Releases a cache. If the cache is associated with cache_keys when the buffer is allocated, the actual release is deferred until all cache_keys are released. The references to cache_keys can be removed in the following two ways: removed after the Decode calls pull_cache successfully; removed when the Prompt calls remove_cache_key.|
|remove_cache_key|Removes a cache_key. After the cache_key is removed, the cache can no longer be pulled by pull_cache. This API can be called only when LLMRole is PROMPT. When LLMRole is DECODER, it does not need to be called because cache_key is not required when allocate_cache is called.|
|pull_cache|Pulls the KV from the corresponding Prompt node to the local KV cache based on CacheKey. This API can be called only when LLMRole is DECODER. The CacheKey must be consistent with the CacheKey of allocate_cache.|
|copy_cache|Copies the KV cache. When you want pull_cache to be pipelined with other operations that use the KV cache, you can allocate an additional transit cache. When other processes are using the KV cache, you can first pull the next KV to the transit cache, and after the other processes finish using the KV cache, copy it to the specified location, thereby hiding the pull_cache latency and reducing the total latency. In the common prefix scenario, before inference of a new request, the common prefix can be copied to new memory and merged with the KV of the current request for inference.|
|allocate_blocks_cache|Allocates a cache of multiple blocks in the PagedAttention scenario.|
|pull_blocks|API for pulling the KV in the PA scenario. The difference from pull_cache is that pull_blocks pulls the KV cache at the corresponding position by block_index.|
|copy_blocks|API for copying the KV in the PA scenario. The difference from copy_cache is that copy_blocks copies the KV cache at the corresponding position by block_index. There is also a difference in usage scenarios: copy_blocks is mainly used when multiple responses need to share the same block and the block is not full, in which case newly added tokens need to be copied to a new block for continued iteration.|
|swap_blocks|Swaps the KV buffer in and out at the corresponding block_index in the PA scenario. It is mainly intended for scenarios where users need to manage the KV buffer by themselves.|
|transfer_cache_async|Initiated by the Prompt node to transfer the cache data to Decode.|
|push_blocks|API for pushing the KV in the PA scenario. It pushes the KV from the corresponding Prompt node to the remote KV cache.|
|push_cache|Pushes the KV from the corresponding Prompt node to the remote KV cache.|

**Scenarios**

It is mainly used for KV cache transmission between distributed clusters.

**Function Example (General Cache Transmission Scenario)**

This example describes the use of APIs in the general cache transmission scenario, mainly involving the allocation, release, and transmission of the KV cache. Pseudo-code examples are provided below based on service roles.

1. The P side and the D side complete the initialization and link establishment of LLMDataDist according to the example in the [Link Management](functions.md#link-management) section.
2. On the P side, apply for a KV Cache buffer of the corresponding size for each request. Taking torch as an example, convert the KV Cache into a torch tensor and perform full model inference.

    ```python
    import torchair
    import torch
    import torch_npu
    # Obtain kv_cache_manager from the initialized llm_datadist.
    kv_cache_manager = llm_datadist.kv_cache_manager
    # Create CacheDesc based on the shape and total number of KV Caches in the model. The shape here is only an example; fill in the actual KV cache shape in the network.
    cache_desc = CacheDesc(num_tensors=4, shape=[4, 4, 8], data_type=DataType.DT_FLOAT16)
    # Create the cache_key of the corresponding request based on the cluster_id used when initializing llm_datadist. When the P side uses a multi-batch model, create cache_keys for the number of batches.
    batch_num = 8
    kv_cache = kv_cache_manager.allocate_cache(cache_desc, [CacheKey(prompt_cluster_id=0, req_id=i, model_id=0) for i in range(batch_num)])

    # Convert the applied KV Cache into a torch tensor.
    kv_tensor_addrs = kv_cache.per_device_tensor_addrs[0]
    kv_tensors = torchair.llm_datadist.create_npu_tensors(kv_cache.cache_desc.shape, torch.float16, kv_tensor_addrs)
    # Pass the converted kv_tensors to the model inference computation to generate the KV Cache, and pass the model output to the incremental inference model as input.
    ```

3. On the D side, apply for a KV Cache buffer for model execution.

    ```python
    # Obtain kv_cache_manager from the initialized llm_datadist.
    kv_cache_manager = llm_datadist.kv_cache_manager
    # Create CacheDesc based on the shape and total number of KV Caches in the model.
    cache_desc = CacheDesc(num_tensors=4, shape=[4, 4, 8], data_type=DataType.DT_FLOAT16)
    # Call the allocate_cache API to apply for the KV Cache buffer of the corresponding request.
    kv_cache = kv_cache_manager.allocate_cache(cache_desc)
    ```

4. Transfer the KV Cache from the P side to the D side in either of the following two ways:
    - On the D side, call the pull\_cache API to pull the KV Cache of the corresponding request into the allocated buffer.

        ```python
        # Create the same cache_key as that used when applying for the cache on the P side to pull the corresponding KV Cache
        cache_key = CacheKey(prompt_cluster_id=0, req_id=1, model_id=0)
        kv_cache_manager.pull_cache(cache_key, kv_cache, batch_index=1) # Pull to the position with batch index 1
        ```

    - On the P side, call the transfer\_cache\_async API to transfer the data to Decode.

        ```python
        from llm_datadist import LayerSynchronizer, TransferConfig


        class LayerSynchronizerImpl(LayerSynchronizer):
            def __init__(self, events):
                self._events = events

            def synchronize_layer(self, layer_index: int, timeout_in_millis: Optional[int]) -> bool:
                self._events[layer_index].wait()
                return True


        events = [torch.npu.Event() for _ in range(cache_desc.num_tensors // 2)]
        # Execute the model. After the computation of each layer is complete, the model calls events[layer_index].record() to record the completion status
        # Model execution is implemented by the user
        # user_model.Predict(kv_tensors, events)

        # After the model is dispatched, call transfer_cache_async to transfer the data. Here, the buffer addresses of the tensors of each layer of the KV Cache already allocated by Decode need to be filled in
        transfer_config = TransferConfig(DECODER_CLUSTER_ID, decoder_kv_cache_addrs)
        cache_task = kv_cache_manager.transfer_cache_async(kv_cache, LayerSynchronizerImpl(events), [transfer_config])
        # Synchronously wait for the transfer result
        cache_task.synchronize()
        ```

5. Using torch as an example, convert the KV Cache into a torch tensor for incremental model inference.

    ```python
    # Convert the allocated KV Cache into the KV Cache type of the framework. Each framework must provide an API for creating a KV Cache of the corresponding type based on the KV Cache address. PyTorch is used as an example here.
    # The conversion operation and the pull operation can be performed in any order.
    kv_tensor_addrs = kv_cache.per_device_tensor_addrs[0]
    kv_tensors = torchair.llm_datadist.create_npu_tensors(kv_cache.cache_desc.shape, torch.float16, kv_tensor_addrs)
    # Split the converted tensor into the KV pairing mode required by the framework. The KV combination can be customized.
    mid = len(kv_tensors) // 2
    k_tensors = kv_tensors[: mid]
    v_tensors = kv_tensors[mid:]
    kv_cache_tensors = list(zip(k_tensors, v_tensors))

    # Pass the converted kv_tensors to the model for iterative inference.
    # Wait for the incremental inference of the request to complete.
    ```

6. Release the KV Cache buffer of the corresponding request based on the timing of cache usage in the service.

    ```python
    # P side: After the KV Cache of all requests in the batch is pulled, calling deallocate_cache actually releases the buffer. If the D side has not pulled the KV Cache, remove_cache_key must also be called.
    kv_cache_manager.remove_cache_key(cache_key_0)
    kv_cache_manager.remove_cache_key(cache_key_2)
    kv_cache_manager.deallocate_cache(kv_cache)
    ```

    ```python
    # D side: Since cache_key is not required during allocation, only the deallocate API needs to be called for release.
    kv_cache_manager.deallocate_cache(kv_cache)
    ```

7. When the service exits, the P side and the D side perform link disconnection according to the example in the [Link Management](functions.md#link-management) section, and then call the finalize API to release resources.

**Function Example (Blocks Cache Transmission Scenario)**

This example describes how to use the APIs in the Blocks Cache (managing the cache in block form) transmission scenario, mainly involving the allocation, release, and transmission of KV Cache. Pseudo-code examples are provided below based on service roles.

1. On the P side and D side, complete the initialization and link establishment of LLMDataDist by following the example in [Link Management](functions.md#link-management).
2. In each layer of the P-side and D-side models, call AllocateCache to allocate KV Cache according to the calculated number of num\_blocks. In the Blocks Cache scenario, different requests reuse the KV Cache of the created num\_blocks size, and the upper-layer framework manages it independently. The allocated buffer is released after the service ends.

    ```python
    # P side
    # Obtain kv_cache_manager from the initialized llm_datadist
    kv_cache_manager = llm_datadist.kv_cache_manager
    # Create CacheDesc based on the shape and total number of KV Cache in the model. The KV cache shape in the PA scenario is usually [num_blocks, block_size,...,...]
    num_blocks = 10
    block_mem_size = 128
    cache_desc = CacheDesc(num_tensors=4, shape=[num_blocks, block_mem_size], data_type=DataType.DT_FLOAT16)
    # Create the BlocksCacheKey of the corresponding request based on the cluster_id used when initializing llm_datadist
    cache_key = BlocksCacheKey(prompt_cluster_id=0, model_id=0)
    # Call the allocate_blocks_cache API to allocate KV Cache buffer
    kv_cache = kv_cache_manager.allocate_blocks_cache(cache_desc, cache_key)

    # Convert the allocated KV Cache to the KV Cache type in the framework. Different frameworks need to provide an API for creating the corresponding KV Cache type based on the KV Cache address. PyTorch is used as an example here
    kv_tensor_addrs = kv_cache.per_device_tensor_addrs[0]
    kv_tensors = torchair.llm_datadist.create_npu_tensors(kv_cache.cache_desc.shape, torch.float16, kv_tensor_addrs)
    ```

    ```python
    # D side
    kv_cache_manager = llm_datadist.kv_cache_manager
    num_blocks = 10
    block_mem_size = 128
    cache_desc = CacheDesc(num_tensors=4, shape=[10, 128], data_type=DataType.DT_FLOAT16)
    kv_cache = kv_cache_manager.allocate_blocks_cache(cache_desc)

    kv_tensor_addrs = kv_cache.per_device_tensor_addrs[0]
    kv_tensors = torchair.llm_datadist.create_npu_tensors(kv_cache.cache_desc.shape, torch.float16, kv_tensor_addrs)
    ```

3. After a new request arrives on the P side, the inference framework assigns the corresponding block\_index to each request. After model inference is complete, the KV Cache corresponding to the request resides in the buffer where the corresponding block\_index is located. The model output and the block\_table corresponding to the request are then transmitted to the D-side inference model as input.
4. After a new request arrives on the D side, the inference framework also assigns the corresponding block\_index to each request, and then calls the pull\_blocks API to transmit the KV Cache to the specified location based on the correspondence between the P-side block\_index and the D-side block\_index. There are two approaches:
    - Call the pull\_blocks API on the D side to pull the KV Cache.

        ```python
        # On the D side, add a new request based on the information passed from the P side, and apply for the corresponding block_table
        # On the D side, pull the KV to the corresponding blocks based on the src_block_table of the passed request and the newly applied dst_block_table
        cache_key = BlocksCacheKey(prompt_cluster_id=0, model_id=0) # Input parameter when allocate_blocks_cache is called on the P side
        kv_cache_manager.pull_blocks(cache_key, cache, [0, 1], [2, 3]) # Pull the data at block positions 0 and 1 on the P side to block positions 2 and 3 on the D side
        ```

    - Transmit the data to the D side when the transfer\_cache\_async API is called on the P side.

        ```python
        # Implement LayerSynchronizerImpl to obtain the completion status of each layer's computation through torch Events. In this example, the Event mechanism is used
        class LayerSynchronizerImpl(LayerSynchronizer):
            def __init__(self, events):
                self._events = events

            def synchronize_layer(self, layer_index: int, timeout_in_millis: Optional[int]) -> bool:
                self._events[layer_index].wait()
                return True


        events = [torch.npu.Event() for _ in range(cache_desc.num_tensors // 2)]
        # Execute the model. After the computation of each layer is complete, the model calls events[layer_index].record() to record the completion status
        # This function is implemented by the user.
        user_model.Predict(kv_cache_tensors, events)

        # After model delivery is complete, call transfer_cache_async to transmit data. Fill in the buffer addresses of the KV Cache tensors of each layer allocated by Decode.
        transfer_config = TransferConfig(DECODER_CLUSTER_ID, decoder_kv_cache_addrs)
        cache_task = kv_cache_manager.transfer_cache_async(kv_cache, LayerSynchronizerImpl(events), [transfer_config], [0, 1],
                                                           [2, 3])
        # Wait synchronously for the transmission result.
        cache_task.synchronize()
        ```

5. After the service ends, the P side and D side call deallocate\_cache to release the allocated KV Cache buffer.

    ```python
    # Wait for the D side to finish pulling the KV Cache of the corresponding request.
    # Release the KV Cache of the corresponding request based on the timing of cache usage in the service. In the PA scenario, the corresponding cache_key does not need to be released.
    kv_cache_manager.deallocate_cache(kv_cache)
    ```

6. When the service exits, the P side and D side perform link disconnection and llm\_datadist finalize according to the cluster link disconnection example.

**Exception Handling**

- Error code LLM\_DEVICE\_OUT\_OF\_MEMORY indicates that the Device failed to allocate the KV Cache buffer. Check the ge.flowGraphMemMaxSize value set during initialization and the size of the allocated KV Cache, and verify whether any requested KV Cache has not been released after being pulled.
- Error code LLM\_KV\_CACHE\_NOT\_EXIST indicates that the peer KV Cache does not exist. Check whether the peer process is abnormal or whether the inference of the corresponding KV Cache request has completed. This error does not affect other request processes. After confirming the process, you can retry.
- Error code <!--__BS_33__--> indicates a pull KV timeout, which means a problem has occurred on the link. You need to disconnect and re-establish the link before retrying.
