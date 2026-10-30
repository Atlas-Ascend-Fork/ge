# get\_ret\_code

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:57:40.301Z pushedAt=2026-10-29T03:13:29.710Z nl=2 -->

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
