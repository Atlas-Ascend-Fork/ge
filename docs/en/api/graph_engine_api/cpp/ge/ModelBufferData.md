# ModelBufferData

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:22:42.970Z pushedAt=2026-10-28T07:40:22.166Z nl=2 -->

Serialized model data in a memory buffer. The header file is located at **include/ge/ge\_ir\_build.h** under the file storage path after CANN software installation.

```c++
struct ModelBufferData
{
  std::shared_ptr<uint8_t> data = nullptr;
  uint64_t length;
};
```

**data** points to the generated model data, and **length** represents the actual size of the model.
