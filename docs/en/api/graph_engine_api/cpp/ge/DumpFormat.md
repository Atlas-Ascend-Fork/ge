# DumpFormat

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:17:47.852Z pushedAt=2026-10-28T07:40:22.143Z nl=2 -->

Format of the Dump file. The header file is located at **include/graph/graph.h** in the file storage path after CANN software installation.

```c++
enum class DumpFormat : uint32_t {
  kOnnx,      // ONNX-based model description structure, which can be opened with visualization tools such as Netron. The generated file is named ge_onnx*.pbtxt.
  kTxt,       // Text file stored in protobuf format. The generated file is named ge_proto*.txt.
  kReadable   // Highly readable text file in Dynamo style. The generated file is named ge_readable*.txt.
};
```
