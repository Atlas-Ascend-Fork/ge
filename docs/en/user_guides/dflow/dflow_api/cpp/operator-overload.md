# Relational Operator Overloading

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:04:05.931Z pushedAt=2026-10-29T02:02:12.845Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

For scenarios where AscendString objects need to be compared by size (for example, sorting keys in a map data structure), the following relational operators are overloaded.

## Prototype

```cpp
bool operator<(const AscendString &d) const
bool operator>(const AscendString &d) const
bool operator<=(const AscendString &d) const
bool operator>=(const AscendString &d) const
bool operator==(const AscendString &d) const
bool operator!=(const AscendString &d) const
```

## Parameters

None.

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
