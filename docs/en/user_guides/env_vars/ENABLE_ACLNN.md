# ENABLE_ACLNN

<!-- md-trans-meta sourceCommit=2461b5215e5b8745a550d67c26b5f9840c83434b translatedAt=2026-10-20T12:48:14.370Z pushedAt=2026-10-28T11:30:41.282Z nl=2 -->

## Feature Description

**This environment variable will be deprecated in later versions.**

During graph compilation, setting this environment variable determines whether the host execute function registered for an operator is called during graph execution to implement the host execution logic and kernel delivery. The values are as follows:

- true: During graph execution, the host execute function registered for the operator is called to implement the host execution logic and kernel delivery.
- false: During graph execution, the GE executor still performs the kernel delivery for the operator.
The operator support status is as follows:

- If an operator **only supports** the host-side single-operator execution call API (the aclnnSupport value in the information repository is configured as aclnn_only), setting this environment variable does not take effect, and the host execute function is always called to implement the host execution logic and kernel delivery.
- If an operator **does not support** the host-side single-operator execution call API (the aclnnSupport field is not configured in the information repository), the GE executor is still used to perform the kernel delivery for the operator.
- If an operator supports both methods, the host execute function is called to implement the host execution logic and kernel delivery only when all three conditions are met: the environment variable is set to true, the aclnnSupport flag is set in the information repository (the aclnnSupport value in the information repository is configured as support_aclnn), and the shape is dynamic (the graph shape contains -1 or -2). Otherwise, the GE executor is still used to perform the kernel delivery for the operator.

## Configuration Example

```bash
export ENABLE_ACLNN=true
```

## Usage Constraints

None

## Applicable Products

Supported on all chips
