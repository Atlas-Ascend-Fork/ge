# end\_time

<!-- md-trans-meta sourceCommit=537d343d7af6b25d23bdc38c9e133ec1c0991c0d translatedAt=2026-11-16T05:23:53.520Z pushedAt=2026-11-16T07:53:51.901Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Reads and sets the end time of FlowInfo using the property method.

## Prototype

```python
@property
def end_time(self)
@end_time.setter
def end_time(self, new_value)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| new_value | int | Value of the FlowInfo end time to be set. |

## Return Value

end\_time property.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
flowinfo = df.FlowInfo(...)
flowinfo.end_time = 200
print(flowinfo.end_time)
```

## Constraints

None
