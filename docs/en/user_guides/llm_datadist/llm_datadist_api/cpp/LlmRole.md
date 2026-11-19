# LlmRole

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T09:57:26.112Z pushedAt=2026-11-17T03:39:12.114Z nl=2 -->

Roles in LLM-DataDist

```cpp
enum class LlmRole : int32_t {
  kPrompt = 1,      // role is Prompt
  kDecoder = 2,     // role is Decoder
  kMix = 3,         // role is Mix
  kEnd              // Invalid value
};
```
