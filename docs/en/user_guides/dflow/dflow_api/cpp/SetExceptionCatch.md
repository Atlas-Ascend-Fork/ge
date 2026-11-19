# SetExceptionCatch

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:58:25.293Z pushedAt=2026-11-16T06:04:36.941Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets whether the user exception catching function is enabled.

## Prototype

```cpp
FlowGraph &SetExceptionCatch(bool enable_exception_catch)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| enable_exception_catch | Input | Whether to enable the user exception catch function. The value can be:<br><br>  - **true**: enables the exception function.<br>  - **false**:  disables the exception function.<br><br>Default value: **false**. |

## Return Value

Returns the current object.

## Exception Handling

None.

## Constraints

When the exception function is enabled, you must also use the [SetInputsAlignAttrs](SetInputsAlignAttrs.md) API to enable the data alignment function. Otherwise, a compilation error is reported.
