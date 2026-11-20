# GetRetCode (FlowMsg Data Type)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:01:17.017Z pushedAt=2026-11-16T03:40:31.924Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the error code in the input FlowMsg.

## Prototype

```cpp
int32_t GetRetCode() const
```

## Parameters

None

## Return Value

Returns the error code in the FlowMsg message. 0 indicates success, and a non-zero value indicates an error.

## Exception Handling

None.

## Constraints

None.
