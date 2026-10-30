# get\_exception

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:53:20.356Z pushedAt=2026-10-29T03:11:55.012Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the exception reported by other UDF nodes.

## Prototype

```python
get_exception(self) -> tuple
```

## Parameters

None

## Return Value

Returns tuple\(bool, int, int\). The meaning of each element in the tuple is as follows.

- The first element in the tuple indicates whether an exception exists: True indicates that an exception exists and is captured successfully, and False indicates that no exception exists.
- The second element in the tuple indicates the error code of the user-defined exception in the captured exception.
- The third element in the tuple indicates the context ID of the user-defined exception in the captured exception. It identifies the reported exception so that other UDFs can identify the specific data batch that generated the exception based on this ID after capturing the exception.

## Exception Handling

None

## Constraints

If exception reporting is not enabled for the dataflow graph through the set\_exception\_catch API, the first element of the return value of this API is false.
