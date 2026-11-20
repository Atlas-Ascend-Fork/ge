# CacheDesc

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T09:47:35.977Z pushedAt=2026-11-17T03:39:12.079Z nl=2 -->

Description of the Cache

```cpp
struct CacheDesc {
  CachePlacement placement = CachePlacement::kDevice;    // Memory type
  uint32_t num_tensors = 0U;                             // Number of tensors in the Cache
  DataType data_type = DT_UNDEFINED;                     // Data type of the tensors in the Cache
  std::vector<int64_t> shape;                            // Shape of the tensors in the Cache
  uint8_t reserved[128];                                 // Reserved
};
```
