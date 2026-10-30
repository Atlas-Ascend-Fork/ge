# GetRetCode (FlowMsg Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:41:24.879Z pushedAt=2026-10-29T01:46:12.709Z nl=2 -->

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

```cpp
int32_t GetRetCode() const
```

## Parameters

None

## Return Value

Returns the error code in the FlowMsg message. 0 indicates normal, and a non-zero value indicates an exception.

## Exception Handling

None.

## Constraints

None.
