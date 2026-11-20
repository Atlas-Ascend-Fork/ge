# get\_ret\_code

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:45:02.187Z pushedAt=2026-11-16T08:02:24.651Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the error code in the input FlowMsg message.

## Prototype

```python
get_ret_code() -> int
```

## Parameters

None

## Return Value

Returns the error code in the FlowMsg message. 0 indicates success, and a non-zero value indicates an error.

## Exception Handling

None

## Constraints

None
