# MULTI\_THREAD\_COMPILE

<!-- md-trans-meta sourceCommit=fb605ff2f9146cf0c88492555f75ac4506c57d51 translatedAt=2026-11-13T10:02:15.749Z pushedAt=2026-11-16T06:11:08.651Z nl=2 -->

## Feature Description

This environment variable controls whether single-threaded compilation is used during model conversion.

<!-- @ref: ge/res/docs/zh/user_guides/env_vars/MULTI_THREAD_COMPILE_res.md#id1 -->

- **0**: Enables single-threaded compilation.
- **1**: (Default) Enables multi-threaded compilation.

## Configuration Example

```bash
export MULTI_THREAD_COMPILE=1
```

## Constraints

- This environment variable applies only to the scenario where the ATC tool is used for model conversion to generate offline .om files.
- If this environment variable is set to any other invalid value, undefined behavior may occur.

## Applicable Products

All chips supported
