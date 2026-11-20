# GetProcessPointType

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T02:59:26.381Z pushedAt=2026-11-16T03:40:06.008Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the type of a ProcessPoint.

## Prototype

```cpp
ProcessPointType GetProcessPointType() const
```

## Parameters

None

## Return Value

Returns the type of a ProcessPoint. The type values are as follows:

```cpp
enum class ProcessPointType {
FUNCTION = 0,
GRAPH = 1,
INNER = 2,
FLOW_GRAPH = 3,
INVALID = 4,
};
```

## Exception Handling

None.

## Constraints

None.
