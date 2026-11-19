# set\_route\_label

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:35:13.070Z pushedAt=2026-11-17T03:39:12.054Z nl=2 -->

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
