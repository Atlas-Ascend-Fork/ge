# CacheIndex

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T09:48:38.789Z pushedAt=2026-11-17T06:06:29.141Z nl=2 -->

Index of the cache

```cpp
struct CacheIndex {
  uint64_t cluster_id;        // ID of the cluster where the cache resides
  int64_t cache_id;           // ID of the cache
  uint32_t batch_index;       // Subscript used to specify the batch during PullKvCache.
  uint8_t reserved[128];      // Reserved
};
```
