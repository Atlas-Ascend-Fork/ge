# Debug

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:49:29.236Z pushedAt=2026-10-28T12:30:42.488Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Records Debug-level logs.

## Prototype

```cpp
virtual void Debug(const char *fmt, ...) __attribute__((format(printf, 2, 3))) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| fmt | Input | Log format control string.<br>Value range: non-empty. |
| ... | Input | Optional parameters<br>Parameters matching fmt. |

## Return Value

None.

## Exception Handling

After log flow control is applied, logs are not recorded.

## Constraints

None.
