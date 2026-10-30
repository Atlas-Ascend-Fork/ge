# ENABLE\_DYNAMIC\_SHAPE\_MULTI\_STREAM

<!-- md-trans-meta sourceCommit=2461b5215e5b8745a550d67c26b5f9840c83434b translatedAt=2026-10-20T12:49:27.017Z pushedAt=2026-10-28T11:30:46.837Z nl=2 -->

## Feature Description

During computational graph execution, enabling multi-stream concurrent execution can improve network performance in certain scenarios. Currently, multi-stream concurrent execution is disabled by default. In dynamic shape graph mode, developers can enable multi-stream concurrent execution by setting this environment variable.

- 0: Disables multi-stream concurrent execution. This is the default value.
- 1: Enables multi-stream concurrent execution.

## Configuration Example

The multi-stream concurrent execution feature is disabled by default. To enable it, set this environment variable to "1".

```bash
export ENABLE_DYNAMIC_SHAPE_MULTI_STREAM=1
```

## Constraints

When a static subgraph exists in a dynamic shape graph, the single-stream configuration in the graph compilation config takes precedence over the ENABLE\_DYNAMIC\_SHAPE\_MULTI\_STREAM environment variable.

In other words, if single-stream execution is enabled in the config of the PyTorch graph mode (TorchAir) while the environment variable for multi-stream execution is also enabled, the former takes precedence. An example of the graph mode configuration is as follows:

```python
config.ge_config.enable_single_stream = True
```

## Applicable Products

Supported on all chips
