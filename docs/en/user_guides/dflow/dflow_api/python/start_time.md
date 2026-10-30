# start\_time

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:18:21.312Z pushedAt=2026-10-29T03:23:15.535Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Reads and sets the start time of FlowInfo as a property.

## Prototype

```python
@property
def start_time(self)
@start_time.setter
def start_time(self, new_value)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| new_value | int | Value of the start time of FlowInfo to be set. |

## Return Value

start\_time property.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
flowinfo = FlowInfo(...)
flowinfo.start_time = 100
print(flowinfo.start_time)
```

## Constraints

None
