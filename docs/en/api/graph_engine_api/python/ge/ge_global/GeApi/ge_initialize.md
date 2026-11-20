# ge\_initialize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:59:11.394Z pushedAt=2026-10-16T10:53:07.412Z -->

## Description

Initializes **GE** to prepare for execution.

## Prototype

```python
ge_initialize(config: dict) -> int
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **config** | Input | Configuration dictionary. The key is the parameter type and the value is the parameter value, both in string format. For supported configuration items, see the global-level parameters in [options parameter description](../../../../cpp/ge/options_params/options_parameters_description.md). |

## Return Value

\(int\) Returns the initialization result, where 0 indicates success.

## Constraints

- If **config** is not of the dictionary type, a **TypeError** is thrown.
- If **config** is empty, a **TypeError** is thrown.
- If initialization fails, a **RuntimeError** is thrown.
- GE does not support multi-instance running. Only one instance can be initialized at a time.
- Calling this API multiple times without calling ge\_finalize results in unpredictable behavior.
