# GetRequiredOutputTensor

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:57:21.057Z pushedAt=2026-10-28T08:50:45.428Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

Obtains the Tensor pointer to a required output based on the operator IR prototype definition. This API maps the IR prototype index to the corresponding output instance, which differs from the flat instance index used by [`GetOutputTensor`](GetOutputTensor.md).

## Prototype

```c++
const Tensor *GetRequiredOutputTensor(size_t ir_index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | 0-based output index of the required output in the operator IR prototype definition. |

## Return Value

Returns the read-only Tensor pointer to the corresponding required output. Returns `nullptr` when the Context is abnormal, `ir_index` is out of range, the mapping from IR to instance does not exist, or the output has no instance.

## Constraints

None
