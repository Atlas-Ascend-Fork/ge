# RaiseException (MetaRunContext Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:46:07.548Z pushedAt=2026-11-16T05:51:19.129Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

A UDF actively reports an exception, which can be caught by other UDFs in the same scope.

## Prototype

```cpp
void RaiseException(int32_t expCode, uint64_t userContextId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| expCode | Input | Error code of the user-defined exception. |
| userContextId | Input | Context ID of the user-defined exception. It identifies the reported exception so that other UDFs can identify the specific data batch that generated the exception based on this ID after capturing the exception. |

## Return Value

None

## Exception Handling

Prints an error log.

## Constraints

If exception reporting is not enabled for the current DataFlow graph through **SetExceptionCatch**, calling this API in a UDF causes the process to report an error and exit.

In streaming input scenarios (where the flow func input is a queue), proactive exception reporting by a UDF is not supported.
