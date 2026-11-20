# SetGraphPpBuilderAsync

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T04:00:21.344Z pushedAt=2026-11-16T06:05:10.756Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets whether the Builder of GraphPp in a FlowGraph is executed asynchronously.

## Prototype

```cpp
void SetGraphPpBuilderAsync(bool graphpp_builder_async)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graphpp_builder_async | Input | Whether the Builder of GraphPp in the FlowGraph is executed asynchronously. The value can be:<br><br>  - true: yes<br>  - false: no<br><br>Default value: false |

## Return Value

None

## Exception Handling

None.

## Constraints

None.
