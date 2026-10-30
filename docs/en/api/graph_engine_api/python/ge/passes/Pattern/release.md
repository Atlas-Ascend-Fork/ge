# release

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:00:11.027Z pushedAt=2026-10-16T10:53:07.641Z -->

## Applicable Products

Supported on all chips.

## Description

Releases the Python-side ownership and returns the C++ Pattern\* handle as an integer.

## Prototype

```python
release() -> int
```

## Parameters

None

## Return Value

| Type | Description |
| --- | --- |
| int | Returns the integer value corresponding to the C++ Pattern* handle. |

## Constraints

- This method is for internal use by the GE bridge layer. Python Pass developers do not need to call it directly. The framework automatically calls **release()** after **patterns()** returns Pattern.
- A successful **release** can be called at most once per instance.
- After the call, methods other than is\_valid/release will throw **RuntimeError**.
