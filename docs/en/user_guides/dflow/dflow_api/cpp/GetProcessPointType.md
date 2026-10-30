# GetProcessPointType

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:40:00.552Z pushedAt=2026-10-29T01:45:44.569Z nl=2 -->

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
