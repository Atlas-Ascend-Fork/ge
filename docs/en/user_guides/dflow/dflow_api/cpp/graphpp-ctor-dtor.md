# GraphPp Constructor and Destructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:16:22.230Z pushedAt=2026-11-16T04:52:15.629Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

The constructor and destructor of GraphPp. The constructor returns a GraphPp object.

## Prototype

```cpp
GraphPp(const char_t *pp_name, const GraphBuilder &builder)
~GraphPp() override
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pp_name | Input | Name of the GraphPp, which must be unique across the entire graph. |
| builder | Input | Method for building the GE Graph: `std::function<ge::Graph()`><br>For details about building the GE Graph, see *Graph Mode Development Guide*. |

## Return Value

Returns a GraphPp object.

## Exception Handling

None.

## Constraints

None.
