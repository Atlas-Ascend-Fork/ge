# Cache

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T13:07:33.188Z pushedAt=2026-10-29T03:25:50.708Z nl=2 -->

Cache, which maintains the addresses of a group of tensors

```cpp
struct Cache {
  int64_t cache_id = -1;                     // ID of the Cache
  std::vector<uintptr_t> tensor_addrs;       // Addresses of the tensors in the Cache. In a single-process multi-device scenario, the addresses of multiple devices are arranged in sequence.
  CacheDesc cache_desc;                      // Cache description
  uint8_t reserved[128];                     // Reserved
};
```
