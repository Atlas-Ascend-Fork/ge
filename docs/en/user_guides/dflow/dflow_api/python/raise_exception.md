# raise\_exception

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:17:16.190Z pushedAt=2026-11-17T03:39:12.012Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

A UDF actively reports an exception.

## Prototype

```python
raise_exception(self, exception_code: int, user_context_id: int)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| exception_code | Input | Error code of the user-defined exception. |
| user_context_id | Input | Context ID of the user-defined exception. It identifies the reported exception so that other UDFs can identify the specific data batch that generated the exception based on this ID after capturing the exception. |

## Return Value

None

## Exception Handling

Prints an error log.

## Constraints

If exception reporting is not enabled for the current dataflow graph through the set\_exception\_catch API, calling this API causes the process to report an error and exit.

In streaming input scenarios (where the flow func input parameter is a queue), proactive exception reporting by UDFs is not supported.
