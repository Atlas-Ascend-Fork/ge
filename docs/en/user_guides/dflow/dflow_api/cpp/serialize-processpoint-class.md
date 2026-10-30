# Serialize (ProcessPoint Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:13:04.647Z pushedAt=2026-10-29T02:08:51.707Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

The serialization method of ProcessPoint. The function of this method is implemented by subclasses of ProcessPoint.

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
