# GetException (MetaContext class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T12:08:58.155Z pushedAt=2026-10-28T12:30:42.577Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

UDF obtains exceptions. If the exception capture function is enabled, you need to attempt to capture exceptions at the beginning of the Proc function in the UDF.

## Prototype

```cpp
bool GetException(int32_t &expCode, uint64_t &userContextId);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| expCode | Input | Error code of the user-defined exception in the captured exception. |
| userContextId | Input | Context ID of the user-defined exception in the captured exception. It is used to identify the reported exception, ensuring that other UDFs can identify the specific data batch in which the exception occurred based on this ID value after capturing the exception. |

## Return Value

Whether an exception exists.

- true: An exception exists and has been caught.
- false: No exception exists that needs to be caught.

## Exception Handling

None

## Constraints

If the current DataFlow graph does not enable exception reporting through **SetExceptionCatch**, the return value of **GetException** is always false.
