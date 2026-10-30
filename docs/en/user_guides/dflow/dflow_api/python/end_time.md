# end\_time

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:19:17.214Z pushedAt=2026-10-29T03:06:29.702Z nl=2 -->

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
flowinfo = FlowInfo(...)
flowinfo.end_time = 200
print(flowinfo.end_time)
```

## Constraints

None
