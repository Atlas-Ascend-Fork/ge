# SetGraphPpBuilderAsync

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:44:38.651Z pushedAt=2026-10-29T02:15:42.754Z nl=2 -->

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
