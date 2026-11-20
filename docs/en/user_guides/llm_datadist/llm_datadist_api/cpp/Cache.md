# Cache

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T09:46:46.808Z pushedAt=2026-11-17T03:39:12.076Z nl=2 -->

Cache, which maintains the addresses of a group of tensors

```cpp
struct Cache {
  int64_t cache_id = -1;                     // ID of the Cache
  std::vector<uintptr_t> tensor_addrs;       // Address of each tensor in the Cache. In a single-process multi-card scenario, the addresses of multiple cards are arranged in sequence.
  CacheDesc cache_desc;                      // Cache description
  uint8_t reserved[128];                     // Reserved
};
```
