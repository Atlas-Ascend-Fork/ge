# IsLogEnable

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:18:41.900Z pushedAt=2026-11-16T04:53:03.523Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Queries whether logs of the corresponding level and type are enabled.

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
