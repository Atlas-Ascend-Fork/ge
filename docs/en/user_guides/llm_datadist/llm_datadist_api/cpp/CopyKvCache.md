# CopyKvCache

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:12:34.141Z pushedAt=2026-10-29T03:25:50.719Z nl=2 -->

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

Copies the KV cache. Both D2D and D2H copies are supported.

When pipelining PullKvCache with other operations that use the cache, you can additionally apply for an intermediate cache. While other processes are using the cache, you can first pull the next cache to the intermediate cache, and after the other processes finish using the cache, copy it to the specified location. This hides the PullKvCache latency through pipelining and reduces the total time.

In the common prefix scenario, before inference of a new request, you can copy the common prefix to new memory and merge it with the KV of the current request for inference.

## Prototype

```cpp
Status CopyKvCache(const Cache &src_cache,
                   const Cache &dst_cache,
                   uint32_t src_batch_index = 0U,
                   uint32_t dst_batch_index = 0U,
                   uint64_t offset = 0U,
                   int64_t size = -1)
```

## Parameters

| Parameter | Input/Output | Value Description |
| --- | --- | --- |
| src_cache | Input | Source cache. |
| dst_cache | Input | Destination cache. |
| src_batch_index | Input | Batch index of the source cache. |
| dst_batch_index | Input | Batch index of the destination cache. |
| offset | Input | Copy offset, in bytes. |
| size | Input | Set to an integer greater than 0 to indicate the size to copy.<br>Or set to -1 to indicate a full copy.<br>The default value is -1. |

## Examples

```cpp
Status ret = llm_datadist.CopyKvCache(src_cache, dst_cache, 0, 0);
```

## Return Value

- LLM\_SUCCESS: Success
- LLM\_PARAM\_INVALID: Parameter error
- Others: Failure

## Constraints

Before calling this API, you need to call [Initialize](Initialize.md) to complete initialization. Only the copy between Device-\>Device and Device-\>Host is supported.
