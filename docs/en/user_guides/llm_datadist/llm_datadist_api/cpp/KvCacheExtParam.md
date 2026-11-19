# KvCacheExtParam

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T09:54:02.275Z pushedAt=2026-11-17T03:39:12.104Z nl=2 -->

Extended parameters passed when calling Pull- or Push-related APIs.

```cpp
struct KvCacheExtParam {
  std::pair<int32_t, int32_t> src_layer_range = {-1, -1};  // Layer range on the source side during KV transmission.
  std::pair<int32_t, int32_t> dst_layer_range{-1, -1};  // Layer range on the destination side during KV transmission.
  uint8_t tensor_num_per_layer = 2U;                       // Number of tensors per layer during KV transmission.
  uint8_t reserved[127];                                   // Reserved field.
};
```
