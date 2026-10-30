# LlmRole

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T13:22:03.171Z pushedAt=2026-10-29T03:25:50.744Z nl=2 -->

Roles in LLM-DataDist

```cpp
enum class LlmRole : int32_t {
  kPrompt = 1,      // Role: Prompt
  kDecoder = 2,     // Role: Decoder
  kMix = 3,         // Role: Mix
  kEnd              // Invalid value
};
```
