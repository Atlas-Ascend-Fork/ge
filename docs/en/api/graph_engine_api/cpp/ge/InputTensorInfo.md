# InputTensorInfo

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:21:38.829Z pushedAt=2026-10-28T07:40:22.162Z nl=2 -->

Input tensor information. The header file is located at **include/ge/ge\_api\_types.h** under the file storage path after CANN software installation.

```c++
struct InputTensorInfo {
  uint32_t data_type;               // data type
  std::vector<int64_t> dims;        // shape description
  void *data;                       // tensor data
  int64_t length;                   // tensor length
};
```
