# Relational Operator Overloading

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:25:21.974Z pushedAt=2026-11-16T05:02:38.407Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

For scenarios where the sizes of AscendString objects are compared (for example, sorting keys in a map data structure), the following relational operators are overloaded.

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
