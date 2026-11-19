# FlowData Constructor and Destructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:25:26.928Z pushedAt=2026-11-16T06:11:08.747Z nl=2 -->

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
