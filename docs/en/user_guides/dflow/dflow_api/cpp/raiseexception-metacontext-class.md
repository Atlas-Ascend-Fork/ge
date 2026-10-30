# RaiseException (MetaContext Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:06:40.177Z pushedAt=2026-10-29T02:03:27.697Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

A UDF proactively reports an exception, which can be captured by other UDFs in the same scope.

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

Prints error logs.

## Constraints

If exception reporting is not enabled for the current DataFlow graph through **SetExceptionCatch**, calling this API in a UDF causes the process to report an error and exit.

In streaming input scenarios (that is, when the flow func input is a queue), proactive exception reporting by UDFs is not supported.
