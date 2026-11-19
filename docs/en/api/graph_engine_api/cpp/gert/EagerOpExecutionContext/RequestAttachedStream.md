# RequestAttachedStream

<!-- md-trans-meta sourceCommit=15a778ecf7e4b3afeaf2e614d2b2a251acddc449 translatedAt=2026-11-17T02:43:53.642Z pushedAt=2026-11-18T02:41:08.879Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Requests a framework-managed physical auxiliary stream by key, allowing Eager custom operators to request an auxiliary stream in the [`Execute`](../../ge/EagerExecuteOp/Execute.md) callback and dispatch kernels on the auxiliary stream.

- On the first request, the framework creates the physical stream and manages its lifecycle. Subsequent requests with the same key within the same model execution instance directly return the already-created stream handle, supporting reuse across nodes and operator types.
- Different keys return different stream handles. Auxiliary streams of different model execution instances are isolated from each other and are not shared even if the keys are the same.

## Prototype

```c++
rtStream RequestAttachedStream(const ge::AscendString &key, uint32_t flags)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| key | Input | Key for auxiliary stream reuse. It cannot be an empty string; nullptr is returned if it is empty. |
| flags | Input | Stream creation flag, a bitwise combination of raw values (such as `RT_STREAM_PERSISTENT` and `RT_STREAM_FORCE_COPY`). It takes effect only when the satellite stream is created for the first time and is ignored on a reuse hit. Set it to 0 when no special flag is required, in which case the stream is created with the framework's default creation parameters. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | rtStream | Returns the framework-managed auxiliary stream handle on success; returns nullptr if the key is empty, the current execution path does not support auxiliary stream requests, or physical stream creation/binding fails. |

## Constraints

- The execution order between the main and auxiliary streams is managed by the operator itself, for example, by synchronizing through events (`aclrtRecordEvent`/`aclrtStreamWaitEvent`). The framework does not guarantee the execution order.
- The operator must join back the work on the auxiliary stream to the execution stream returned by `GetStream()` before [`Execute`](../../ge/EagerExecuteOp/Execute.md) returns (record an event on the auxiliary stream and wait for the event on the execution stream). Memory reclamation on the RT2 path only perceives the framework logical stream. If the operator returns without joining back, the arguments and output memory allocated in this round may be reclaimed and reused before the auxiliary stream kernel finishes execution.
- The stream lifecycle is managed by the framework. The operator must not destroy the returned auxiliary stream (calling `aclrtDestroyStream` is prohibited).
- This API does not support concurrent calls.
- `CANN-FMK-` is the auxiliary stream key prefix reserved for internal framework use. User-defined operators should avoid using this prefix for keys to prevent overwriting the auxiliary streams requested by the framework.

## Examples

```c++
ge::graphStatus Execute(gert::EagerOpExecutionContext *ctx) override {
  // Request the framework-managed satellite stream by key and specify the stream creation flag.
  gert::rtStream aux_stream = ctx->RequestAttachedStream("eager_aux",
                                                         RT_STREAM_PERSISTENT | RT_STREAM_FORCE_COPY);
  if (aux_stream == nullptr) {
    return ge::GRAPH_FAILED;
  }
  // Request again with the same key: the same handle is returned within the same model instance (flags are ignored in this case).
  gert::rtStream reuse_stream = ctx->RequestAttachedStream("eager_aux", 0U);
  if (reuse_stream != aux_stream) {
    return ge::GRAPH_FAILED;
  }

  const auto main_stream = static_cast<aclrtStream>(ctx->GetStream());
  // Record an event on the main stream to ensure that the kernel on the auxiliary stream executes after the input is ready.
  aclError ret = aclrtRecordEvent(ev_main_to_aux, main_stream);
  if (ret != ACL_SUCCESS) {
    return ge::GRAPH_FAILED;
  }
  ret = aclrtStreamWaitEvent(static_cast<aclrtStream>(aux_stream), ev_main_to_aux);
  if (ret != ACL_SUCCESS) {
    return ge::GRAPH_FAILED;
  }

  // Dispatch the kernel to the auxiliary stream (instead of the main stream) for execution.
  ret = aclrtLaunchKernelV2(func_handle, num_blocks, dev_args, sizeof(args), nullptr,
                             static_cast<aclrtStream>(aux_stream));
  if (ret != ACL_SUCCESS) {
    return ge::GRAPH_FAILED;
  }

  // Record an event on the auxiliary stream and wait on the main stream to ensure that the output is used after the kernel completes.
  ret = aclrtRecordEvent(ev_aux_to_main, static_cast<aclrtStream>(aux_stream));
  if (ret != ACL_SUCCESS) {
    return ge::GRAPH_FAILED;
  }
  ret = aclrtStreamWaitEvent(main_stream, ev_aux_to_main);
  if (ret != ACL_SUCCESS) {
    return ge::GRAPH_FAILED;
  }
  return ge::GRAPH_SUCCESS;
}
```
