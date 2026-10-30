# IsLogEnable

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:57:31.516Z pushedAt=2026-10-29T01:54:44.344Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Queries whether logging is enabled for the corresponding level and type.

## Prototype

```cpp
virtual bool IsLogEnable(FlowFuncLogLevel level) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| level | Input | The log level is of the FlowFuncLogLevel type.<br>enum class FLOW_FUNC_VISIBILITY FlowFuncLogLevel {<br>DEBUG = 0,<br>INFO = 1,<br>WARN = 2,<br>ERROR = 3<br>}; |

## Return Value

Whether the log level can be recorded.

## Exception Handling

None.

## Constraints

None.
