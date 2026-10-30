# CopyKvBlocks

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:12:04.008Z pushedAt=2026-10-29T03:25:50.717Z nl=2 -->

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

In PA scenarios, this API copies the KV cache by block list. It supports D2D, D2H, and H2D copies.

- The D2D scenario mainly applies when multiple responses need to share the same block and the block is not full. In this case, newly added tokens need to be copied to a new block to continue iteration.
- H2D and D2H copies are mainly used for swapping cache memory in and out on the corresponding block\_index.

## Prototype

```cpp
Status CopyKvBlocks(const Cache &src_cache,
                    const Cache &dst_cache,
                    const std::vector<uint64_t> &src_blocks,
                    const std::vector<std::vector<uint64_t>> &dst_blocks_list)
```

## Parameters

| Parameter | Input/Output | Value Description |
| --- | --- | --- |
| src_cache | Input | Source cache. |
| dst_cache | Input | Destination cache. |
| src_blocks | Input | Block index list of the source cache. |
| dst_blocks_list | Input | List of block index lists of the destination cache. One group of src_blocks can be copied to multiple groups of dst_blocks. |

## Examples

```cpp
Status ret = llm_datadist.CopyKvBlocks(src_cache, dst_cache, {1,2}, {{1,2},{3,4}});
```

## Return Value

- LLM\_SUCCESS: Success
- LLM\_PARAM\_INVALID: Invalid parameter
- Others: Failure

## Constraints

Before calling this API, call [Initialize](Initialize.md) to complete initialization. Copy of Host-\>Host is not supported.
