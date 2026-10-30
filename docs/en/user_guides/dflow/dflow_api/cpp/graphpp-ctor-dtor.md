# GraphPp Constructor and Destructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:56:16.058Z pushedAt=2026-10-29T01:54:04.818Z nl=2 -->

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
| builder | Input | Method for building the GE Graph: `std::function<ge::Graph()`><br>For details about how to build the GE Graph, see [*Graph Development*](../../../graph_dev/README.md). |

## Return Value

Returns a GraphPp object.

## Exception Handling

None.

## Constraints

None.
