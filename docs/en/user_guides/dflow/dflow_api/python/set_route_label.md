# set\_route\_label

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:16:45.021Z pushedAt=2026-10-29T03:22:53.315Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the route label.

For example, the udf1 and udf2 nodes are deployed on device1 and device2 respectively, and each has two outputs (out0 and out1). If you want each pair of out0 and out1 to be sent to the same device, set the same route\_label for the two output flowmsgs.

## Prototype

```python
set_route_label(self, route_label) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| route_label | Input | Label of the route. The value 0 indicates that it is not used. |

## Return Value

None

## Exception Handling

None

## Constraints

None
