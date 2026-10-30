# GetGraphBuilder (FlowGraphPp Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T12:10:49.247Z pushedAt=2026-10-28T12:30:42.584Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the function for creating a Graph in FlowGraphPp.

## Prototype

```cpp
GraphBuilder GetGraphBuilder() const
```

## Parameters

None

## Return Value

Function for creating a Graph. The function prototype is as follows.

```cpp
std::function<ge::Graph()>
```

## Exception Handling

None.

## Constraints

None.
