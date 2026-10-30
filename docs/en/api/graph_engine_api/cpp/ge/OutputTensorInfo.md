# OutputTensorInfo

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:23:29.358Z pushedAt=2026-10-28T07:40:22.169Z nl=2 -->

Describes the output tensor information. The header file is located in the file storage path after CANN software installation: **include/ge/ge\_api\_types.h**.

```c++
struct OutputTensorInfo {
  uint32_t data_type{};             // data type
  std::vector<int64_t> dims;        // shape description
  std::unique_ptr<uint8_t[]> data;  // tensor data
  int64_t length{};                 // tensor length
  OutputTensorInfo() : dims({}), data(nullptr) {}
  OutputTensorInfo(OutputTensorInfo &&out) noexcept = default;
  OutputTensorInfo &operator=(OutputTensorInfo &&out)& noexcept {
    if (this != &out) {
      data_type = out.data_type;
      dims = out.dims;
      data = std::move(out.data);
      length = out.length;
    }
    return *this;
  }
  OutputTensorInfo(const OutputTensorInfo &) = delete;
  OutputTensorInfo &operator=(const OutputTensorInfo &)& = delete;
};
```
