# MAX\_COMPILE\_CORE\_NUMBER

<!-- md-trans-meta sourceCommit=2461b5215e5b8745a550d67c26b5f9840c83434b translatedAt=2026-10-20T12:52:15.081Z pushedAt=2026-10-28T11:35:59.192Z nl=2 -->

## Feature Description

This environment variable specifies the number of CPU cores available during graph compilation.

This environment variable must be set to an integer, which indicates that multithreading is enabled during graph compilation, with the number of threads equal to the configured number of CPU cores.

Value range: 1\~CPU cores. The default value is 1, which means multithreading is disabled. If the value is less than or equal to 0, the default value 1 is used.

It is recommended that the value of this environment variable not exceed the number of CPU cores in the current environment; otherwise, performance may degrade.

## Configuration Example

```bash
export MAX_COMPILE_CORE_NUMBER=5
```

## Applicable Products

All chips supported
