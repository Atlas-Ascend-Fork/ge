# TE\_PARALLEL\_COMPILER

<!-- md-trans-meta sourceCommit=3b4c8f106b74c3a75512dd245c2b55b143c90db0 translatedAt=2026-10-20T12:54:31.015Z pushedAt=2026-10-28T11:50:26.539Z nl=2 -->

## Feature Description

Maximum number of processes for parallel operator compilation. When this parameter is greater than 1, the system enables parallel compilation.

When the network model is large, you can configure this environment variable to enable parallel operator compilation.

## Configuration Example

```bash
export TE_PARALLEL_COMPILER=8
```

## Constraints

The maximum value does not exceed the number of CPU cores \*80%/AI processors, the value range is 1\~32, and the default value is 8.

## Applicable Products

All chips supported
