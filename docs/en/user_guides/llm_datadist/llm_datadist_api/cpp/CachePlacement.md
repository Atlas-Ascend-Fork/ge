# CachePlacement

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T09:49:07.255Z pushedAt=2026-11-17T03:39:12.082Z nl=2 -->

Memory type of the cache

```cpp
enum class CachePlacement : uint32_t {
  kHost = 0U,             // Cache is Host buffer
  kDevice = 1U,           // Cache is Device buffer
};
```
