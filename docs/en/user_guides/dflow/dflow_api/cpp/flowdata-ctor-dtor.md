# FlowData Constructor and Destructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:56:14.347Z pushedAt=2026-10-28T12:30:42.517Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

FlowData constructor and destructor. The constructor returns a FlowData node.

## Prototype

```cpp
FlowData(const char *name, int64_t index)
~FlowData() override
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the input node, which must be unique across the entire graph. |
| index | Input | Index of the input node. The input indexes in the entire graph must be accumulated starting from 0, indicating the order of inputs in the graph. |

## Return Value

Returns a FlowData node.

## Exception Handling

None.

## Constraints

None.
