# Serialize (ProcessPoint Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:51:46.219Z pushedAt=2026-11-16T05:59:57.486Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

The serialization method of ProcessPoint. The function of this method is implemented by child classes of ProcessPoint.

## Prototype

```cpp
virtual void Serialize(ge::AscendString &str) const = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| str | Output | String after ProcessPoint serialization. |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
