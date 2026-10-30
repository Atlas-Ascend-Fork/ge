# GetLogger

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:37:37.872Z pushedAt=2026-10-29T01:44:27.103Z nl=2 -->

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
