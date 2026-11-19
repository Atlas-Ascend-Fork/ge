# CopyKvBlocks

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:49:40.199Z pushedAt=2026-11-17T06:09:42.978Z nl=2 -->

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

In PA scenarios, this API copies the KV cache using the block list. It supports D2D, D2H, and H2D copy operations.

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
| src_blocks | Input | Block index list of the source Cache. |
| dst_blocks_list | Input | List of block index lists of the destination cache. One group of src_blocks can be copied to multiple groups of dst_blocks. |

## Examples

```cpp
Status ret = llm_datadist.CopyKvBlocks(src_cache, dst_cache, {1,2}, {{1,2},{3,4}});
```

## Return Value

- LLM\_SUCCESS: Success
- LLM\_PARAM\_INVALID: Parameter error
- Others: Failure

## Constraints

Before calling this API, call [Initialize](Initialize.md) to complete initialization. Copy of Host-\>Host is not supported.
