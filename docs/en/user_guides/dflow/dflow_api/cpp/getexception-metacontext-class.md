# GetException (MetaContext class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T02:51:25.719Z pushedAt=2026-11-16T03:37:14.549Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

UDF gets exceptions. If the exception catch feature is enabled, an attempt to catch exceptions is required at the beginning of the Proc function in the UDF.

## Prototype

```cpp
bool GetException(int32_t &expCode, uint64_t &userContextId);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| expCode | Input | Error code of the user-defined exception in the captured exception. |
| userContextId | Input | Context ID of the user-defined exception in the captured exception. It identifies the reported exception so that other UDFs can identify the specific data batch that generated the exception based on this ID after capturing the exception. |

## Return Value

Whether an exception exists.

- true: An exception exists and has been caught.
- false: No exception needs to be caught.

## Exception Handling

None

## Constraints

If exception reporting is not enabled for the current DataFlow graph through **SetExceptionCatch**, the return value of **GetException** is always false.
