# CachePlacement

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T13:10:31.861Z pushedAt=2026-10-29T03:25:50.713Z nl=2 -->

Memory type of the cache

```cpp
enum class CachePlacement : uint32_t {
  kHost = 0U,             // Cache is host memory
  kDevice = 1U,           // Cache is device memory
};
```
