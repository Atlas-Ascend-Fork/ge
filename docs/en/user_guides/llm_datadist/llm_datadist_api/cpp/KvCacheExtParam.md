# KvCacheExtParam

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T13:19:38.358Z pushedAt=2026-10-29T03:25:50.734Z nl=2 -->

Extended parameters passed when calling Pull- or Push-related APIs.

```cpp
struct KvCacheExtParam {
  std::pair<int32_t, int32_t> src_layer_range = {-1, -1};  // Layer range at the source end during KV transfer
  std::pair<int32_t, int32_t> dst_layer_range{-1, -1};  // Layer range at the destination end during KV transfer
  uint8_t tensor_num_per_layer = 2U;                       // Number of tensors per layer during KV transfer
  uint8_t reserved[127];                                   // Reserved field
};
```
