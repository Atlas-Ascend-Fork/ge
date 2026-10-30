# IGNORE\_INFER\_ERROR

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-20T12:52:03.519Z pushedAt=2026-10-28T11:35:52.936Z nl=2 -->

## Function Description

**This environment variable is an experimental feature and may change in later versions.**

Switch for skipping the operator prototype deliverable validation when integrating operators into a graph. Deliverables include the implementation of operator graph integration adaptation functions such as shape inference.

- Non-empty: When the deliverable for integrating operators into a graph is missing, the validation error is skipped during operator compilation, allowing the use case to recover quickly.
- Not configured: (Default) When the deliverable for integrating operators into a graph is missing, operator compilation reports an error.

## Configuration Example

The value "1" in the following example is only a reference sample. You can set it to any other non-empty string.

```bash
export IGNORE_INFER_ERROR=1
```

## Constraints

This environment variable is not recommended. It is intended only for scenarios where the deliverables for integrating user-defined operators into a graph are missing and a compilation error occurs, and you want to quickly restore the use case.

If the deliverables for integrating custom operators into a graph are missing (for example, the InferDataType implementation is absent), compilation failures or precision issues may occur. You are advised to complete them by referring to the "Programming Guide \> Appendix \> Integrating Operators into a Graph (GE Graph) Development" section in the *[Ascend C Operator Development](https://gitcode.com/cann/asc-devkit/blob/9.2.0/docs/zh/guide/index.md)* manual.

## Applicable Products

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
Supported on all chips
<!-- end id1 -->

<!-- @ref: ge/res/docs/zh/user_guides/env_vars/IGNORE_INFER_ERROR_res.md#id1 -->
