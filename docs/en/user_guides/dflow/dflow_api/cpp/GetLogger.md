# GetLogger

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T02:56:38.642Z pushedAt=2026-11-16T03:39:27.375Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the log implementation class.

## Prototype

```cpp
static FlowFuncLogger &GetLogger(FlowFuncLogType type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| type | Input | Log type, which is FlowFuncLogType<br>enum class FLOW_FUNC_VISIBILITY FlowFuncLogType {<br>DEBUG_LOG = 0, // Debug log<br>RUN_LOG = 1  // Run log<br>}; |

## Return Value

Log implementation class.

## Exception Handling

None.

## Constraints

None.
