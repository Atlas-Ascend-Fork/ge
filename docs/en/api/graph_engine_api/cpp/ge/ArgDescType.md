# ArgDescType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:13:45.188Z pushedAt=2026-10-28T07:40:22.134Z nl=2 -->

Type of the Args address. The header file is located in **include/graph/arg\_desc\_info.h** under the file storage path after CANN software installation.

```c++
enum class ArgDescType {
  kIrInput = 0, // Input.
  kIrOutput, // Output.
  kWorkspace, // Workspace address.
  kTiling, // Tiling address.
  kHiddenInput, // Additional input not expressed on the IR.
  kCustomValue, // Custom content.
  kIrInputDesc, // Input address with description information.
  kIrOutputDesc, // Output address with description information.
  kInputInstance, // Instantiated input.
  kOutputInstance, // Instantiated output.
  kEnd
};
```
