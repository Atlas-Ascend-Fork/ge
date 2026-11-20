# DeclareResourceUsage

<!-- md-trans-meta sourceCommit=f386938e32e869b0e9685baebe312ea95158e291 translatedAt=2026-11-17T03:02:19.767Z pushedAt=2026-11-18T02:41:08.929Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: libgraph.so

## Description

Declares the attached stream resources used by a custom operator node during graph compilation. GE invokes this API once for each custom operator node in the graph that implements it, and the operator reports the resources that the node will request at runtime.

## Prototype

```c++
virtual graphStatus DeclareResourceUsage(gert::ResourceUsageContext &ctx) = 0
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ctx | Input | Resource declaration context. You can obtain node information and report resource usage. It is valid only during the callback. For details, see [`gert::ResourceUsageContext`](../../gert/ResourceUsageContext/overview.md). |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | graphStatus | `GRAPH_SUCCESS(0)`: Declaration succeeded. Other values: Declaration failed. |

## Constraints

None

## Examples

```c++
#include <string>

#include "graph/custom_op.h"

class MyEagerOp : public ge::EagerExecuteOp, public ge::ResourceUsageReporter {
 public:
  ge::graphStatus Execute(gert::EagerOpExecutionContext *ctx) override {
    const auto *input = ctx->GetInputTensor(0U);
    if (input == nullptr) {
      return ge::GRAPH_FAILED;
    }
    // Request the attached stream by key at runtime (the key is consistent with that declared in DeclareResourceUsage).
    gert::rtStream aux_stream =
        ctx->RequestAttachedStream(ge::AscendString(MakeKey(input->GetShape().GetStorageShape().GetDim(0U)).c_str()),
                                   0U);
    if (aux_stream == nullptr) {
      return ge::GRAPH_FAILED;
    }
    // ... Launch the kernel on the attached stream and synchronize with the main stream through an event ...
    return ge::GRAPH_SUCCESS;
  }

  ge::graphStatus DeclareResourceUsage(gert::ResourceUsageContext &ctx) override {
    // Declare all keys to be used at runtime by bucketing based on the static shape of the node at compile time.
    const auto *input = ctx.GetInputTensor(0U);
    if (input == nullptr) {
      return ge::GRAPH_FAILED;
    }
    const std::vector<ge::AscendString> keys{
        ge::AscendString(MakeKey(input->GetShape().GetStorageShape().GetDim(0U)).c_str())};
    return ctx.ReportAttachedStream(keys);
  }

 private:
  static std::string MakeKey(int64_t dim0) {
    return "my_op_aux_" + std::to_string(dim0);
  }
};
```

In the preceding example, `RequestAttachedStream` at runtime and `ReportAttachedStream` at compile time share the same key generation logic to ensure that the declaration is consistent with the runtime request. For two nodes with input shapes of 1024 and 2048 respectively, the model statistic is 2.
