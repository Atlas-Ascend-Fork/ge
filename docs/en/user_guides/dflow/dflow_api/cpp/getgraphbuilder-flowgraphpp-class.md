# GetGraphBuilder (FlowGraphPp Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T02:53:22.636Z pushedAt=2026-11-16T03:38:17.731Z nl=2 -->

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
