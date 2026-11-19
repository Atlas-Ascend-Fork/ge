# Session Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:18:48.208Z pushedAt=2026-10-16T10:53:07.726Z -->

## Description

Initializes a **Session**.

## Prototype

```python
Session(options: Optional[dict] = None)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| options | Input | Configuration option dictionary, an optional parameter. For supported configuration items, see the session-level parameters in [Options Parameter Description](../../../../cpp/ge/options_params/options_parameters_description.md). |

## Return Value

None

## Constraints

- If **options** is not of the dictionary type, a **TypeError** is thrown.
- If session creation fails, a **RuntimeError** is thrown.
