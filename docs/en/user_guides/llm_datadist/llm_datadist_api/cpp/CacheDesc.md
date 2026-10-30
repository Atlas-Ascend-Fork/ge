# CacheDesc

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T13:09:00.991Z pushedAt=2026-10-29T03:25:50.709Z nl=2 -->

Description of the cache

```cpp
struct CacheDesc {
  CachePlacement placement = CachePlacement::kDevice;    // Memory type
  uint32_t num_tensors = 0U;                             // Number of tensors in the cache
  DataType data_type = DT_UNDEFINED;                     // Data type of the tensors in the cache
  std::vector<int64_t> shape;                            // Shape of the tensors in the cache
  uint8_t reserved[128];                                 // Reserved
};
```
