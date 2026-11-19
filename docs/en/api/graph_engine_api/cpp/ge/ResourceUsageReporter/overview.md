# Overview

<!-- md-trans-meta sourceCommit=ff42e70ef4dd6e3da4c079832d1983d4ff97dea4 translatedAt=2026-10-26T13:11:38.783Z pushedAt=2026-10-28T07:42:19.786Z nl=2 -->

`ge::ResourceUsageReporter` is the resource declaration API for custom operators. After an operator implements `DeclareResourceUsage`, GE calls this API per node during the graph compilation phase, allowing the operator to declare the resources it will use at runtime.

This API targets the auxiliary stream resource observation scenario for Eager-type custom operators (those that implement [`EagerExecuteOp`](../EagerExecuteOp/overview.md) and request an auxiliary stream in `Execute`):

- The statistics are a purely compile-time out-of-band observation capability. They do not change stream allocation, memory planning, or runtime execution behavior, and the statistical values are not a resource quota or reservation commitment.
- Operators that do not implement this API are not included in the statistics, and their compilation and execution are unaffected.
- For declarative operators (nodes whose `ArgsRefreshStrategy` is `kAnnotatedArgs`), resource usage is precisely counted by the framework, and GE does not call `DeclareResourceUsage` for such nodes. Even if an operator implements both `AnnotatedArgsOp` and `ResourceUsageReporter`, it is still processed through the declarative path to avoid double counting.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
virtual graphStatus DeclareResourceUsage(gert::ResourceUsageContext &ctx) = 0
```

## Related APIs

- [`DeclareResourceUsage`](DeclareResourceUsage.md)
- [`gert::ResourceUsageContext`](../../gert/ResourceUsageContext/overview.md)
- [`gert::EagerOpExecutionContext::RequestAttachedStream`](../../gert/EagerOpExecutionContext/RequestAttachedStream.md)
- [`CompiledGraphSummary::GetStreamNum`](../CompiledGraphSummary/GetStreamNum.md)
